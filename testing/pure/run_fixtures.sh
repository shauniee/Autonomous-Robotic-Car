#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cd "$repo_root"

# These pure functions are extracted verbatim from production translation units
# that also contain target-only IRQ/task code. This avoids compiling or
# substituting any T-Kernel, GPIO, timer, or hardware behavior on the host.
extract_function() {
    source_file=$1
    return_type=$2
    symbol=$3
    prelude=$4
    output_file=$5
    printf '%s\n' "$prelude" > "$output_file"
    awk -v ret="$return_type" -v sym="$symbol" '
        function brace_delta(line, rest, count) {
            rest=line; count=0
            while (match(rest, /[{}]/)) {
                if (substr(rest, RSTART, 1) == "{") count++
                else count--
                rest=substr(rest, RSTART + 1)
            }
            return count
        }
        !copying && $0 ~ "^[[:space:]]*" ret "[[:space:]]+" sym "[[:space:]]*\\(" { copying=1 }
        copying {
            print
            delta=brace_delta($0)
            if (index($0, "{") && !started) started=1
            if (started) {
                depth += delta
                if (depth == 0) { found=1; exit }
            }
        }
        END { if (!found) exit 1 }
    ' "$source_file" >> "$output_file"
}

extract_function app_program/subsystem2/drv_encoder.c int drv_encoder_transition \
    '#include <stdint.h>' "$build_dir/encoder_transition.c"
extract_function app_program/subsystem3/mission.c bool mission_mapping_valid \
    '#include "app_program/subsystem3/mission.h"' "$build_dir/mission_mapping_valid.c"
extract_function app_program/subsystem4/subsystem4.c void s4_snapshot_run_reset \
    '#include <stdbool.h>
#include <stdint.h>
#include "app_program/subsystem4/subsystem4.h"
#include "app_program/common/run_history.h"' "$build_dir/snapshot_run_reset.c"

${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
    -DAPP_TEST=1 -D_PICO_RP2040_ -I. -Iinclude -Iconfig -Ikernel/knlinc \
    -include testing/pure/host_test_report.h \
    testing/pure/run_fixture_suite.c \
    testing/subsystem1/subsystem1_test.c \
    testing/subsystem2/subsystem2_test.c \
    testing/subsystem3/subsystem3_test.c \
    testing/subsystem4/subsystem4_test.c \
    testing/subsystem5/subsystem5_test.c \
    app_program/subsystem1/mapping_protocol.c \
    app_program/subsystem1/mapping_ack.c \
    app_program/subsystem1/status_protocol.c \
    app_program/subsystem1/telemetry.c \
    app_program/subsystem2/motion_control.c \
    "$build_dir/encoder_transition.c" \
    app_program/subsystem3/ir_tracking.c \
    app_program/subsystem3/ir_tracking_calibration.c \
    app_program/subsystem3/joint_calibration.c \
    app_program/subsystem3/barcode_decode.c \
    app_program/subsystem3/barcode_service.c \
    app_program/subsystem3/mission_guards.c \
    app_program/subsystem3/ir_snapshot.c \
    "$build_dir/mission_mapping_valid.c" \
    app_program/subsystem4/terrain_algo.c \
    app_program/subsystem4/terrain_calibration.c \
    app_program/subsystem4/terrain_motion.c \
    "$build_dir/snapshot_run_reset.c" \
    app_program/subsystem5/obstacle_profile.c \
    app_program/common/button.c \
    app_program/common/run_history.c \
    app_program/common/robot_config.c \
    -Wl,-dead_strip -lm -o "$build_dir/fixture_suite"
"$build_dir/fixture_suite"

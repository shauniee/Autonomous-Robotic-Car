#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cd "$repo_root"

${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -DAPP_TEST=1 \
    -D_PICO_RP2040_ -include stddef.h -Iinclude -Iconfig -Ikernel/knlinc \
    testing/pure/pure_algorithms.c \
    app_program/subsystem1/mapping_protocol.c \
    app_program/subsystem1/telemetry.c \
    app_program/subsystem2/motion_control.c \
    app_program/subsystem3/barcode_decode.c \
    app_program/subsystem3/barcode_service.c \
    app_program/subsystem3/ir_tracking.c \
    app_program/subsystem4/terrain_algo.c \
    app_program/subsystem5/obstacle_profile.c \
    app_program/common/run_history.c \
    app_program/common/robot_config.c -lm -o "$build_dir/pure_algorithms"
"$build_dir/pure_algorithms"

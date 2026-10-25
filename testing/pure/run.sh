#!/bin/sh
# Host-only tests for kernel-free modules. Run from anywhere: testing/pure/run.sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cd "$repo_root"

CC=${CC:-cc}
CFLAGS="-std=c11 -O2 -Wall -Wextra -Werror -pthread"

$CC $CFLAGS testing/pure/test_robot_time.c -o "$build_dir/test_robot_time"
$CC $CFLAGS testing/pure/test_encoder_math.c -o "$build_dir/test_encoder_math"
$CC $CFLAGS testing/pure/test_motion_control.c app_program/subsystem2/motion_control.c \
    app_program/common/robot_data.c -lm -o "$build_dir/test_motion_control"
$CC $CFLAGS testing/pure/test_barcode.c app_program/subsystem3/barcode_frame.c \
    app_program/subsystem3/barcode_decode.c app_program/common/robot_data.c -o "$build_dir/test_barcode"
$CC $CFLAGS testing/pure/test_safety.c app_program/common/robot_data.c -o "$build_dir/test_safety"
$CC $CFLAGS testing/pure/test_mission.c app_program/subsystem3/mission_logic.c \
    app_program/common/robot_data.c -lm -o "$build_dir/test_mission"
$CC $CFLAGS testing/pure/test_echo_range.c -o "$build_dir/test_echo_range"
$CC $CFLAGS testing/pure/test_obstacle_profile.c app_program/subsystem5/obstacle_profile.c -lm \
    -o "$build_dir/test_obstacle_profile"
$CC $CFLAGS testing/pure/test_mqtt_frame.c app_program/subsystem1/mqtt_frame.c -o "$build_dir/test_mqtt_frame"
$CC $CFLAGS testing/pure/test_robot_data.c app_program/common/robot_data.c \
    -o "$build_dir/test_robot_data"

"$build_dir/test_robot_time"
"$build_dir/test_encoder_math"
"$build_dir/test_robot_data"
"$build_dir/test_motion_control"
"$build_dir/test_barcode"
"$build_dir/test_safety"
"$build_dir/test_mission"
"$build_dir/test_echo_range"
"$build_dir/test_obstacle_profile"
"$build_dir/test_mqtt_frame"

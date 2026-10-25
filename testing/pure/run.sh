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
$CC $CFLAGS testing/pure/test_terrain.c app_program/subsystem4/terrain_algo.c app_program/common/robot_data.c -lm \
    -o "$build_dir/test_terrain"
$CC $CFLAGS testing/pure/test_robot_data.c app_program/common/robot_data.c \
    -o "$build_dir/test_robot_data"

"$build_dir/test_robot_time"
"$build_dir/test_encoder_math"
"$build_dir/test_robot_data"
"$build_dir/test_motion_control"
"$build_dir/test_terrain"

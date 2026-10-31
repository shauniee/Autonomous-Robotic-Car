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
$CC $CFLAGS -DMOTION_STALL_GUARD_ENABLED=1 testing/pure/test_motion_control.c app_program/subsystem2/motion_control.c \
    app_program/common/robot_data.c -lm -o "$build_dir/test_motion_control_guarded"
$CC $CFLAGS testing/pure/test_barcode.c app_program/subsystem3/barcode_decode.c \
    app_program/common/robot_data.c -o "$build_dir/test_barcode"
$CC $CFLAGS testing/pure/test_ir_tracking.c app_program/subsystem3/ir_tracking.c -lm \
    -o "$build_dir/test_ir_tracking"
$CC $CFLAGS testing/pure/test_safety.c app_program/common/robot_data.c -o "$build_dir/test_safety"
$CC $CFLAGS -DMISSION_BYPASS_RELEASED=1 -DMISSION_RECOVERY_RELEASED=1 testing/pure/test_mission.c app_program/mission/mission_logic.c \
    app_program/mission/recovery_control.c app_program/mission/bypass_control.c \
    app_program/subsystem2/motion_control.c app_program/common/robot_data.c -lm -o "$build_dir/test_mission"
$CC $CFLAGS -DMISSION_BYPASS_RELEASED=0 -DMISSION_RECOVERY_RELEASED=0 testing/pure/test_mission_pending.c app_program/mission/mission_logic.c \
    app_program/mission/recovery_control.c app_program/mission/bypass_control.c \
    app_program/subsystem2/motion_control.c app_program/common/robot_data.c -lm -o "$build_dir/test_mission_pending"
$CC $CFLAGS testing/pure/test_mission_mapping.c app_program/mission/mission_mapping.c \
    app_program/common/robot_data.c -o "$build_dir/test_mission_mapping"
$CC $CFLAGS testing/pure/test_echo_range.c -o "$build_dir/test_echo_range"
$CC $CFLAGS testing/pure/test_obstacle_profile.c app_program/subsystem5/obstacle_profile.c \
    -o "$build_dir/test_obstacle_profile"
$CC $CFLAGS testing/pure/test_range_filter.c app_program/subsystem5/range_filter.c \
    -o "$build_dir/test_range_filter"
$CC $CFLAGS testing/pure/test_profile_edges.c app_program/subsystem5/profile_edges.c -lm \
    -o "$build_dir/test_profile_edges"
$CC $CFLAGS testing/pure/test_mqtt_frame.c app_program/subsystem1/mqtt_frame.c \
    app_program/common/robot_data.c -o "$build_dir/test_mqtt_frame"
$CC $CFLAGS testing/pure/test_terrain.c testing/pure/terrain_float_reference.c \
    app_program/subsystem4/terrain_algo.c app_program/common/robot_data.c -lm \
    -o "$build_dir/test_terrain"
$CC $CFLAGS testing/pure/test_s1_report.c app_program/subsystem1/s1_report.c \
    app_program/subsystem1/mqtt_frame.c app_program/common/robot_data.c \
    -o "$build_dir/test_s1_report"
$CC $CFLAGS testing/pure/test_s1_stress.c app_program/subsystem1/s1_bridge.c app_program/common/robot_data.c \
    -o "$build_dir/test_s1_stress"
$CC $CFLAGS testing/pure/test_robot_data.c app_program/common/robot_data.c \
    -o "$build_dir/test_robot_data"

"$build_dir/test_robot_time"
"$build_dir/test_encoder_math"
"$build_dir/test_robot_data"
"$build_dir/test_motion_control"
"$build_dir/test_motion_control_guarded"
$CC $CFLAGS testing/pure/test_motion_trace.c app_program/subsystem2/motion_trace.c \
    app_program/subsystem1/mqtt_frame.c app_program/common/robot_data.c \
    -o "$build_dir/test_motion_trace"
"$build_dir/test_motion_trace"
"$build_dir/test_barcode"
"$build_dir/test_ir_tracking"
"$build_dir/test_safety"
"$build_dir/test_mission"
"$build_dir/test_mission_pending"
"$build_dir/test_mission_mapping"
"$build_dir/test_echo_range"
"$build_dir/test_obstacle_profile"
"$build_dir/test_range_filter"
"$build_dir/test_profile_edges"
"$build_dir/test_mqtt_frame"
"$build_dir/test_terrain"
"$build_dir/test_s1_report"
"$build_dir/test_s1_stress"
python3 scripts/mqtt_client.py --self-test	# laptop decoder agrees with the golden vectors

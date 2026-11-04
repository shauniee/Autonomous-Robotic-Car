#!/bin/sh
# Host-only tests for kernel-free modules. Run from anywhere: testing/pure/run.sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cd "$repo_root"

CC=${CC:-cc}
CFLAGS="-std=c11 -O2 -Wall -Wextra -Werror -pthread -DHOST_TEST"

$CC $CFLAGS testing/pure/test_robot_time.c -o "$build_dir/test_robot_time"
$CC $CFLAGS testing/pure/test_watchdog.c -o "$build_dir/test_watchdog"
"$build_dir/test_watchdog"
for geometry in 0 1; do
    $CC $CFLAGS -DNAV_GEOMETRY_RELEASED=$geometry -DNAV_SENSOR_FORWARD_MM=$((geometry * 150)) -DNAV_CROSSBAR_MM=$((geometry * 18)) \
        testing/pure/test_smooth_navigation.c app_program/mission/mission_logic.c \
        app_program/mission/recovery_control.c app_program/mission/bypass_control.c \
        app_program/motion/motion_control.c app_program/common/robot_data.c -lm -o "$build_dir/test_smooth_$geometry"
    "$build_dir/test_smooth_$geometry"
done
$CC $CFLAGS testing/pure/test_encoder_math.c -o "$build_dir/test_encoder_math"
$CC $CFLAGS testing/pure/test_motion_control.c app_program/motion/motion_control.c \
    app_program/common/robot_data.c -lm -o "$build_dir/test_motion_control"
$CC $CFLAGS -DMOTION_STALL_GUARD_ENABLED=1 testing/pure/test_motion_control.c app_program/motion/motion_control.c \
    app_program/common/robot_data.c -lm -o "$build_dir/test_motion_control_guarded"
$CC $CFLAGS testing/pure/test_barcode.c app_program/line_barcode/barcode.c \
    app_program/common/robot_data.c -o "$build_dir/test_barcode"
$CC $CFLAGS testing/pure/test_ir_tracking.c app_program/line_barcode/line.c -lm \
    -o "$build_dir/test_ir_tracking"
$CC $CFLAGS testing/pure/test_line_replay.c app_program/line_barcode/line.c app_program/mission/mission_logic.c \
    app_program/mission/recovery_control.c app_program/mission/bypass_control.c \
    app_program/motion/motion_control.c app_program/common/robot_data.c -lm -o "$build_dir/test_line_replay"
$CC $CFLAGS testing/pure/test_safety.c app_program/common/robot_data.c -o "$build_dir/test_safety"
$CC $CFLAGS -DMISSION_BYPASS_RELEASED=1 -DMISSION_RECOVERY_RELEASED=1 testing/pure/test_mission.c app_program/mission/mission_logic.c \
    app_program/mission/recovery_control.c app_program/mission/bypass_control.c \
    app_program/motion/motion_control.c app_program/common/robot_data.c -lm -o "$build_dir/test_mission"
$CC $CFLAGS -DMISSION_BYPASS_RELEASED=0 -DMISSION_RECOVERY_RELEASED=0 testing/pure/test_mission_pending.c app_program/mission/mission_logic.c \
    app_program/mission/recovery_control.c app_program/mission/bypass_control.c \
    app_program/motion/motion_control.c app_program/common/robot_data.c -lm -o "$build_dir/test_mission_pending"
$CC $CFLAGS testing/pure/test_mission_mapping.c app_program/mission/mission_mapping.c \
    app_program/common/robot_data.c -o "$build_dir/test_mission_mapping"
$CC $CFLAGS testing/pure/test_echo_range.c -o "$build_dir/test_echo_range"
$CC $CFLAGS testing/pure/test_obstacle_profile.c app_program/obstacle/obstacle_profile.c \
    -o "$build_dir/test_obstacle_profile"
$CC $CFLAGS testing/pure/test_range_filter.c app_program/obstacle/obstacle_profile.c \
    -o "$build_dir/test_range_filter"
$CC $CFLAGS testing/pure/test_profile_edges.c app_program/obstacle/obstacle_profile.c -lm \
    -o "$build_dir/test_profile_edges"
$CC $CFLAGS testing/pure/test_mqtt_frame.c app_program/comms/mqtt_frame.c \
    app_program/common/robot_data.c -o "$build_dir/test_mqtt_frame"
$CC $CFLAGS testing/pure/test_terrain.c \
    app_program/terrain/terrain_algo.c app_program/common/robot_data.c -lm \
    -o "$build_dir/test_terrain"
$CC $CFLAGS testing/pure/test_comms_report.c app_program/comms/report.c \
    app_program/comms/mqtt_frame.c app_program/common/robot_data.c \
    -o "$build_dir/test_comms_report"
$CC $CFLAGS testing/pure/test_comms_stress.c app_program/comms/report.c \
    app_program/comms/mqtt_frame.c app_program/common/robot_data.c \
    -o "$build_dir/test_comms_stress"
$CC $CFLAGS testing/pure/test_robot_data.c app_program/common/robot_data.c \
    -o "$build_dir/test_robot_data"

"$build_dir/test_robot_time"
"$build_dir/test_encoder_math"
"$build_dir/test_robot_data"
"$build_dir/test_motion_control"
"$build_dir/test_motion_control_guarded"
$CC $CFLAGS testing/pure/test_motion_trace.c app_program/motion/motion.c \
    app_program/comms/mqtt_frame.c app_program/common/robot_data.c \
    -o "$build_dir/test_motion_trace"
"$build_dir/test_motion_trace"
$CC $CFLAGS testing/pure/test_line_trace.c app_program/mission/line_trace.c \
    app_program/comms/mqtt_frame.c app_program/common/robot_data.c \
    -o "$build_dir/test_line_trace"
"$build_dir/test_line_trace"
"$build_dir/test_barcode"
"$build_dir/test_ir_tracking"
"$build_dir/test_line_replay"
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
"$build_dir/test_comms_report"
"$build_dir/test_comms_stress"
python3 testing/pure/test_mqtt_cli.py	# rejected ACKs and burst-safe speed estimate
python3 scripts/mqtt_client.py --self-test	# laptop decoder agrees with the golden vectors
python3 testing/pure/test_check_barr.py	# coding rule scan finds each violation, passes clean code
python3 testing/pure/test_field_tools.py	# capture triage and recorded lap scoring
python3 testing/pure/test_capture_imu.py

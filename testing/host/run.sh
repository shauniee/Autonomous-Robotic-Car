#!/bin/sh
# Host-only tests for kernel-free modules. Prints one line per test and the
# full output of any test that fails. Run from anywhere: testing/host/run.sh
set -u

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cd "$repo_root"

CC=${CC:-cc}
CFLAGS="-std=c11 -O2 -Wall -Wextra -Werror -pthread -DHOST_TEST"
T=testing/host
DATA=app_program/common/robot_data.c
FRAME="app_program/comms/mqtt_frame.c $DATA"
MISSION="app_program/mission/mission_logic.c app_program/mission/recovery_control.c
    app_program/mission/bypass_control.c app_program/motion/motion_control.c $DATA"
PROFILE=app_program/obstacle/obstacle_profile.c
passed=0
failed=0

report() {
    if [ "$2" -eq 0 ]; then
        echo "PASS $1"
        passed=$((passed + 1))
    else
        echo "FAIL $1"
        sed 's/^/    /' "$build_dir/$1.log"
        failed=$((failed + 1))
    fi
}

# c_test NAME [-DFLAG...] SOURCE...: compile and run one C test binary.
c_test() {
    name=$1
    shift
    # shellcheck disable=SC2086 # CFLAGS is several words
    if $CC $CFLAGS "$@" -lm -o "$build_dir/$name" >"$build_dir/$name.log" 2>&1; then
        "$build_dir/$name" >>"$build_dir/$name.log" 2>&1
        report "$name" $?
    else
        report "$name" 1
    fi
}

# py_test NAME COMMAND...: run one Python check.
py_test() {
    name=$1
    shift
    python3 "$@" >"$build_dir/$name.log" 2>&1
    report "$name" $?
}

# shellcheck disable=SC2086 # the source lists are several words
{
c_test robot_time $T/test_robot_time.c
c_test trace_ring $T/test_trace_ring.c
c_test encoder_math $T/test_encoder_math.c
c_test robot_data $T/test_robot_data.c $DATA
c_test motion_control $T/test_motion_control.c app_program/motion/motion_control.c $DATA
c_test motion_control_stall_guard -DMOTION_STALL_GUARD_ENABLED=1 \
    $T/test_motion_control.c app_program/motion/motion_control.c $DATA
c_test motion_trace $T/test_motion_trace.c app_program/motion/motion.c $FRAME
c_test line_trace $T/test_line_trace.c app_program/mission/line_trace.c $FRAME
c_test barcode $T/test_barcode.c app_program/line_barcode/barcode.c $DATA
c_test ir_tracking $T/test_ir_tracking.c app_program/line_barcode/line.c
c_test line_replay $T/test_line_replay.c app_program/line_barcode/line.c $MISSION
c_test safety $T/test_safety.c $DATA
c_test mission -DMISSION_BYPASS_RELEASED=1 -DMISSION_RECOVERY_RELEASED=1 \
    $T/test_mission.c $MISSION
c_test mission_unreleased -DMISSION_BYPASS_RELEASED=0 -DMISSION_RECOVERY_RELEASED=0 \
    $T/test_mission_pending.c $MISSION
c_test smooth_navigation -DNAV_GEOMETRY_RELEASED=0 -DNAV_SENSOR_FORWARD_MM=0 \
    -DNAV_CROSSBAR_MM=0 $T/test_smooth_navigation.c $MISSION
c_test smooth_navigation_geometry -DNAV_GEOMETRY_RELEASED=1 -DNAV_SENSOR_FORWARD_MM=150 \
    -DNAV_CROSSBAR_MM=18 $T/test_smooth_navigation.c $MISSION
c_test mission_mapping $T/test_mission_mapping.c app_program/mission/mission_mapping.c $DATA
c_test echo_range $T/test_echo_range.c
c_test obstacle_profile $T/test_obstacle_profile.c $PROFILE
c_test range_filter $T/test_range_filter.c $PROFILE
c_test profile_edges $T/test_profile_edges.c $PROFILE
c_test mqtt_frame $T/test_mqtt_frame.c $FRAME
c_test terrain $T/test_terrain.c app_program/terrain/terrain_algo.c $DATA
c_test comms_report $T/test_comms_report.c app_program/comms/report.c $FRAME
c_test comms_stress $T/test_comms_stress.c app_program/comms/report.c $FRAME
}

py_test mqtt_cli $T/test_mqtt_cli.py
py_test dashboard_http $T/test_dashboard_http.py
py_test mqtt_decoder scripts/mqtt_client.py --self-test
py_test check_barr $T/test_check_barr.py
py_test check_deliverables $T/test_check_deliverables.py
py_test field_tools $T/test_field_tools.py
py_test capture_imu $T/test_capture_imu.py

echo "host tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]

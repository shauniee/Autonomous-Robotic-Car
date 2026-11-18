#!/bin/sh
# Host-only tests for kernel-free modules. Prints one line per test and the
# full output of any test that fails. Run from anywhere:
#   sh testing/tools/run.sh          every test (CI)
#   sh testing/tools/run.sh S1-U01   only the tests in that test ID's folder
#   sh testing/tools/run.sh tools    only the tool checks in testing/tools/
set -u
only=${1:-}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cd "$repo_root"

CC=${CC:-cc}
CFLAGS="-std=c11 -O2 -Wall -Wextra -Werror -pthread -DHOST_TEST"
C=testing/comms/logs
M=testing/motion/logs
L=testing/line_barcode/logs
TR=testing/terrain/logs
O=testing/obstacle/logs
I=testing/integration/logs
S=testing/system/logs
T=testing/tools
DATA=app_program/common/robot_data.c
FRAME="app_program/comms/mqtt_frame.c $DATA"
MISSION="app_program/mission/mission_logic.c app_program/mission/recovery_control.c
    app_program/mission/bypass_control.c app_program/motion/motion_control.c $DATA"
PROFILE=app_program/obstacle/obstacle_profile.c
passed=0
failed=0
ran=0

# wanted FILE: true when no ID was given or FILE sits in the chosen folder.
wanted() {
    case "$1" in
        *"/$only/"*) return 0 ;;
    esac
    [ -z "$only" ]
}

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
    src=
    for a in "$@"; do case "$a" in -*) ;; *) src=$a; break ;; esac; done
    wanted "$src" || return 0
    ran=$((ran + 1))
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
    wanted "$1" || return 0
    ran=$((ran + 1))
    python3 "$@" >"$build_dir/$name.log" 2>&1
    report "$name" $?
}

# shellcheck disable=SC2086 # the source lists are several words
{
c_test robot_time $S/SYS-04/test_robot_time.c
c_test trace_ring $S/SYS-04/test_trace_ring.c
c_test encoder_math $M/S2-U01/test_encoder_math.c
c_test robot_data $S/SYS-04/test_robot_data.c $DATA
c_test motion_control $M/S2-U05/test_motion_control.c app_program/motion/motion_control.c $DATA
c_test motion_control_stall_guard -DMOTION_STALL_GUARD_ENABLED=1 \
    $M/S2-U05/test_motion_control.c app_program/motion/motion_control.c $DATA
c_test motion_trace $M/S2-U04/test_motion_trace.c app_program/motion/motion.c $FRAME
c_test line_trace $I/INT-01/test_line_trace.c app_program/mission/line_trace.c $FRAME
c_test barcode $L/S3-U03/test_barcode.c app_program/line_barcode/barcode.c $DATA
c_test ir_tracking $L/S3-U01/test_ir_tracking.c app_program/line_barcode/line.c
c_test line_replay $I/INT-01/test_line_replay.c app_program/line_barcode/line.c $MISSION
c_test safety $S/SYS-06/test_safety.c $DATA
c_test mission -DMISSION_BYPASS_RELEASED=1 -DMISSION_RECOVERY_RELEASED=1 \
    $I/INT-09/test_mission.c $MISSION
c_test mission_unreleased -DMISSION_BYPASS_RELEASED=0 -DMISSION_RECOVERY_RELEASED=0 \
    $I/INT-09/test_mission_pending.c $MISSION
c_test smooth_navigation -DNAV_GEOMETRY_RELEASED=0 -DNAV_SENSOR_FORWARD_MM=0 \
    -DNAV_CROSSBAR_MM=0 $I/INT-09/test_smooth_navigation.c $MISSION
c_test smooth_navigation_geometry -DNAV_GEOMETRY_RELEASED=1 -DNAV_SENSOR_FORWARD_MM=150 \
    -DNAV_CROSSBAR_MM=18 $I/INT-09/test_smooth_navigation.c $MISSION
c_test mission_mapping $C/S1-U02/test_mission_mapping.c app_program/mission/mission_mapping.c $DATA
c_test echo_range $O/S5-U01/test_echo_range.c
c_test obstacle_profile $O/S5-U03/test_obstacle_profile.c $PROFILE
c_test range_filter $O/S5-U05/test_range_filter.c $PROFILE
c_test profile_edges $O/S5-U03/test_profile_edges.c $PROFILE
c_test mqtt_frame $C/S1-U05/test_mqtt_frame.c $FRAME
c_test terrain $TR/S4-U02/test_terrain.c app_program/terrain/terrain_algo.c $DATA
c_test comms_report $C/S1-U01/test_comms_report.c app_program/comms/report.c $FRAME
c_test comms_stress $C/S1-U01/test_comms_stress.c app_program/comms/report.c $FRAME
}

py_test mqtt_cli $C/S1-U04/test_mqtt_cli.py
py_test dashboard_http $C/S1-U01/test_dashboard_http.py
py_test mqtt_decoder scripts/mqtt_client.py --self-test
py_test check_barr $T/test_check_barr.py
py_test check_deliverables $T/test_check_deliverables.py
py_test field_tools $T/test_field_tools.py
py_test capture_imu $T/test_capture_imu.py

# Dashboard script in a fake DOM; needs Node, which CI installs.
if wanted $C/S1-U01/test_dashboard.mjs; then
    ran=$((ran + 1))
    node $C/S1-U01/test_dashboard.mjs >"$build_dir/dashboard.log" 2>&1
    report dashboard $?
fi

if [ "$ran" -eq 0 ]; then
    echo "no tests for $only"
    exit 1
fi
echo "host tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]

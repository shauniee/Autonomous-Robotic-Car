#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build-host
s3=app_program/subsystem3
s2=app_program/subsystem2
cc -std=c11 -Wall -Wextra -Werror -pedantic -I"$s3" "$s3/ir_tracking.c" tests/test_ir_tracking.c -lm -o build-host/test_ir_tracking
build-host/test_ir_tracking
cc -std=c11 -Wall -Wextra -Werror -pedantic -DBENCH_HOST_TEST -I"$s3" -Itests/fakes "$s3/drv_ir.c" tests/test_drv_ir.c -o build-host/test_drv_ir
build-host/test_drv_ir
cc -std=c11 -Wall -Wextra -Werror -pedantic -I"$s3" "$s3/ir_tracking.c" "$s3/robot_control.c" tests/test_robot_control.c -lm -o build-host/test_robot_control
build-host/test_robot_control
cc -std=c11 -Wall -Wextra -Werror -pedantic -I"$s3" -I"$s2" "$s3/ir_tracking.c" "$s3/robot_control.c" "$s3/watchdog_policy.c" "$s2/motion_policy.c" tests/test_mtkernel_bench.c -lm -o build-host/test_mtkernel_bench
build-host/test_mtkernel_bench
for profile in 0 1; do
    cc -std=c11 -Wall -Wextra -Werror -pedantic -DBENCH_KERNEL_HOST_TEST -DAPP_TEST="$profile" -I"$s3" -I"$s2" -Itests/fakes "$s3/task_ir.c" "$s3/ir_tracking.c" "$s3/robot_control.c" "$s3/watchdog_policy.c" tests/test_ir_task.c -lm -o "build-host/test_ir_task_$profile"
    "build-host/test_ir_task_$profile"
    cc -std=c11 -Wall -Wextra -Werror -pedantic -DBENCH_KERNEL_HOST_TEST -DAPP_TEST="$profile" -I"$s2" -Itests/fakes "$s2/subsystem2.c" "$s2/motion_policy.c" tests/test_motion_service.c -lm -o "build-host/test_motion_service_$profile"
    "build-host/test_motion_service_$profile"
done

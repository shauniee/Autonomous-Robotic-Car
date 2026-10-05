#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build-host
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude src/sensor_bench_math.c src/sensor_bench_json.c \
    src/controller.c src/comms_protocol.c tests/test_sensor_bench.c -lm -o build-host/test_sensor_bench
build-host/test_sensor_bench
python3 -c 'import json; s=json.load(open("build-host/bench-fixture.json")); assert s["motors"]["enabled"] is False; assert s["imu"]["yaw_rate_dps"] is None; print("Telemetry JSON parsed successfully.")'
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude -Itests/bench_fakes \
    src/sensor_bench.c src/sensor_bench_math.c tests/test_sensor_driver.c -lm -o build-host/test_sensor_driver
build-host/test_sensor_driver
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude -Itests/fakes \
    src/controller.c src/comms_protocol.c src/subsystems/comms_picow.c tests/test_comms_network.c -lm -o build-host/test_network
build-host/test_network
build-host/test_network init-failure
build-host/test_network allocation-failure
cc -std=c11 -Wall -Wextra -Werror -pedantic -DROBOT_TEST_WIFI_ONLY -Iinclude -Itests/fakes \
    src/controller.c src/comms_protocol.c src/subsystems/comms_picow.c tests/test_comms_network.c -lm -o build-host/test_wifi_only
build-host/test_wifi_only
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude src/controller.c tests/test_controller.c -lm -o build-host/test_controller
build-host/test_controller

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    New-Item -ItemType Directory -Force -Path build | Out-Null
    $sources = @(
        'src/controller.c', 'src/comms_protocol.c', 'src/subsystems/comms.c', 'src/subsystems/motion.c',
        'src/subsystems/line_barcode.c', 'src/subsystems/terrain.c',
        'src/subsystems/obstacle.c'
    )
    $flags = @('-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic', '-Iinclude')
    & gcc @flags @sources tests/test_controller.c -o build/test_controller.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed.' }
    & ./build/test_controller.exe
    if ($LASTEXITCODE -ne 0) { throw 'Controller tests failed.' }
    & gcc @flags src/controller.c src/comms_protocol.c tests/test_comms_protocol.c -o build/test_comms_protocol.exe
    if ($LASTEXITCODE -ne 0) { throw 'Protocol test compilation failed.' }
    & ./build/test_comms_protocol.exe
    if ($LASTEXITCODE -ne 0) { throw 'Protocol tests failed.' }
    & gcc @flags -Itests/fakes src/controller.c src/comms_protocol.c src/subsystems/comms_picow.c tests/test_comms_network.c -o build/test_comms_network.exe
    if ($LASTEXITCODE -ne 0) { throw 'Network test compilation failed.' }
    & ./build/test_comms_network.exe
    if ($LASTEXITCODE -ne 0) { throw 'Network tests failed.' }
    foreach ($scenario in @('init-failure', 'allocation-failure')) {
        & ./build/test_comms_network.exe $scenario
        if ($LASTEXITCODE -ne 0) { throw "Network $scenario test failed." }
    }
    & gcc @flags -Itests/fakes -DROBOT_TEST_WIFI_ONLY src/controller.c src/comms_protocol.c src/subsystems/comms_picow.c tests/test_comms_network.c -o build/test_comms_wifi_only.exe
    if ($LASTEXITCODE -ne 0) { throw 'WiFi-only test compilation failed.' }
    & ./build/test_comms_wifi_only.exe
    if ($LASTEXITCODE -ne 0) { throw 'WiFi-only tests failed.' }
    & gcc @flags @sources sim/main.c -o build/robot_sim.exe
    if ($LASTEXITCODE -ne 0) { throw 'Demo compilation failed.' }
    & ./build/robot_sim.exe
    if ($LASTEXITCODE -ne 0) { throw 'Demo failed.' }
} finally {
    Pop-Location
}

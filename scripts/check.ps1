$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    New-Item -ItemType Directory -Force -Path build | Out-Null
    $sources = @(
        'src/controller.c', 'src/subsystems/comms.c', 'src/subsystems/motion.c',
        'src/subsystems/line_barcode.c', 'src/subsystems/terrain.c',
        'src/subsystems/obstacle.c'
    )
    $flags = @('-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic', '-Iinclude')
    & gcc @flags @sources tests/test_controller.c -o build/test_controller.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed.' }
    & ./build/test_controller.exe
    if ($LASTEXITCODE -ne 0) { throw 'Controller tests failed.' }
    & gcc @flags @sources sim/main.c -o build/robot_sim.exe
    if ($LASTEXITCODE -ne 0) { throw 'Demo compilation failed.' }
    & ./build/robot_sim.exe
    if ($LASTEXITCODE -ne 0) { throw 'Demo failed.' }
} finally {
    Pop-Location
}

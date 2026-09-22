param(
    [string]$BuildDir = 'build-picow',
    [string]$NetworkConfig = 'config/network_config.h'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$savedPath = $env:PATH
$savedPythonPath = $env:PYTHONPATH
$savedSdk = $env:PICO_SDK_PATH
$savedToolchain = $env:PICO_TOOLCHAIN_PATH
Push-Location $projectRoot
try {
    # Reuse installed tools or the project-local tools downloaded for validation.
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeCommand) { $cmake = $cmakeCommand.Source }
    elseif (Test-Path 'tmp/build-tools/cmake/data/bin/cmake.exe') {
        $cmake = (Resolve-Path 'tmp/build-tools/cmake/data/bin/cmake.exe').Path
    } else { throw 'Install CMake and Ninja, then run again.' }
    if (Test-Path 'tmp/build-tools/bin') {
        $env:PATH = (Resolve-Path 'tmp/build-tools/bin').Path + ';' + $env:PATH
        $env:PYTHONPATH = (Resolve-Path 'tmp/build-tools').Path + ';' + $env:PYTHONPATH
    }
    if (-not $env:PICO_SDK_PATH -and (Test-Path 'tmp/pico-sdk')) {
        $env:PICO_SDK_PATH = (Resolve-Path 'tmp/pico-sdk').Path
    }
    if (-not $env:PICO_TOOLCHAIN_PATH -and (Test-Path 'tmp/arm-toolchain/bin/arm-none-eabi-gcc.exe')) {
        $env:PICO_TOOLCHAIN_PATH = (Resolve-Path 'tmp/arm-toolchain').Path
    }
    $configPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($NetworkConfig)
    $arguments = @('-S', '.', '-B', $BuildDir, '-G', 'Ninja', '-DPICO_BOARD=pico_w',
        '-DROBOT_ENABLE_WIFI=ON', "-DROBOT_NETWORK_CONFIG=$configPath", '-DCMAKE_BUILD_TYPE=Debug')
    if (Test-Path 'tmp/picotool/picotool/picotoolConfig.cmake') {
        $arguments += '-Dpicotool_DIR=' + (Resolve-Path 'tmp/picotool/picotool').Path
    }
    & $cmake @arguments
    if ($LASTEXITCODE -ne 0) { throw 'Pico W configuration failed.' }
    & $cmake --build $BuildDir --parallel 4
    if ($LASTEXITCODE -ne 0) { throw 'Pico W build failed.' }
    Write-Output "Built $BuildDir/robot_car.uf2. Network settings are embedded at build time."
} finally {
    $env:PATH = $savedPath
    $env:PYTHONPATH = $savedPythonPath
    $env:PICO_SDK_PATH = $savedSdk
    $env:PICO_TOOLCHAIN_PATH = $savedToolchain
    Pop-Location
}

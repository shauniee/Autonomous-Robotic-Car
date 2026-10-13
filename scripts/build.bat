@echo off
setlocal

rem Set toolchain, POSIX shell (Git), and SDK paths
set "PATH=C:\Program Files\Git\bin;C:\Program Files\Git\usr\bin;C:\ProgramData\chocolatey\bin;C:\PROGRA~1\RASPBE~2\PICOSD~1.1\GCC-AR~1\bin;C:\PROGRA~1\RASPBE~2\PICOSD~1.1\PICO-S~1;%PATH%"
set "PICO_SDK_PATH=C:/PROGRA~1/RASPBE~2/PICOSD~1.1/pico-sdk"

cd /d "%~dp0..\build_make"

echo Building Autonomous Robotic Car UF2 with Subsystem 4...
make -j4 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=1

if %ERRORLEVEL% equ 0 (
    echo.
    echo ============================================================
    echo BUILD SUCCESSFUL!
    echo Output UF2: build_make\mtk3pico_smp1_usb_cdc_test.uf2
    echo ============================================================
) else (
    echo.
    echo [ERROR] Build failed! Check errors above.
    exit /b %ERRORLEVEL%
)

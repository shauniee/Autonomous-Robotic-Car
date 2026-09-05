cmake_minimum_required(VERSION 3.13)

# Import the user's installed SDK before project() selects the toolchain.
if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to an installed Raspberry Pi Pico SDK.")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
project(robot_car C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
pico_sdk_init()

add_executable(robot_car
    src/main_pico.c
    src/controller.c
    src/subsystems/comms.c
    src/subsystems/motion.c
    src/subsystems/line_barcode.c
    src/subsystems/terrain.c
    src/subsystems/obstacle.c
)
target_include_directories(robot_car PRIVATE include)
target_compile_options(robot_car PRIVATE -Wall -Wextra)
target_link_libraries(robot_car pico_stdlib m)
pico_enable_stdio_usb(robot_car 1)
pico_enable_stdio_uart(robot_car 0)
pico_add_extra_outputs(robot_car)

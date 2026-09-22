cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico_w CACHE STRING "Pico board")
endif()
option(ROBOT_ENABLE_WIFI "Build Pico W WiFi/MQTT backend" ON)
set(ROBOT_NETWORK_CONFIG "${CMAKE_CURRENT_LIST_DIR}/config/network_config.h"
    CACHE FILEPATH "Local WiFi/MQTT configuration header")

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

set(COMMS_SOURCE src/subsystems/comms.c)
if(ROBOT_ENABLE_WIFI)
    if(NOT PICO_CYW43_SUPPORTED)
        message(FATAL_ERROR "WiFi needs Pico W/WH. Use -DPICO_BOARD=pico_w or -DROBOT_ENABLE_WIFI=OFF.")
    endif()
    set(COMMS_SOURCE src/subsystems/comms_picow.c)
    if(EXISTS "${ROBOT_NETWORK_CONFIG}")
        configure_file("${ROBOT_NETWORK_CONFIG}" generated/network_config.h COPYONLY)
    else()
        message(WARNING "No config/network_config.h: firmware builds with networking disabled until configured.")
        configure_file(config/network_config.example.h generated/network_config.h COPYONLY)
    endif()
endif()

add_executable(robot_car
    src/main_pico.c
    src/controller.c
    src/comms_protocol.c
    ${COMMS_SOURCE}
    src/subsystems/motion.c
    src/subsystems/line_barcode.c
    src/subsystems/terrain.c
    src/subsystems/obstacle.c
)
target_include_directories(robot_car PRIVATE include)
target_compile_options(robot_car PRIVATE -Wall -Wextra)
target_link_libraries(robot_car pico_stdlib m)
if(ROBOT_ENABLE_WIFI)
    target_include_directories(robot_car PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(robot_car pico_cyw43_arch_lwip_poll pico_lwip_mqtt pico_unique_id pico_rand)
endif()
# Avoid a long USB printf wait when a host is absent or not consuming data.
target_compile_definitions(robot_car PRIVATE PICO_STDIO_USB_STDOUT_TIMEOUT_US=0)
pico_enable_stdio_usb(robot_car 1)
pico_enable_stdio_uart(robot_car 0)
pico_add_extra_outputs(robot_car)

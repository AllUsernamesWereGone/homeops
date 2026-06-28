# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(bitcoin_miner_pico2w C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(bitcoin_miner_pico2w
    src/main.c
    src/miner.c
    src/sha256.c
    src/bitcoin_job.c
    src/bitcoin_target.c
    src/bitcoin_hex.c
    src/display_st7735.c
    src/display_ui.c
    src/wifi.c
    src/stratum_tcp.c
    src/stratum.c
    src/mqtt_homeops.c
    )

    target_compile_options(bitcoin_miner_pico2w PRIVATE
        -O3
        -DNDEBUG
        -funroll-loops

    )

    target_link_options(bitcoin_miner_pico2w PRIVATE
        -O3

    )

# pull in common dependencies
target_link_libraries(bitcoin_miner_pico2w
    pico_stdlib
    hardware_spi
    hardware_pwm
    )


if (PICO_CYW43_SUPPORTED)
    target_link_libraries(bitcoin_miner_pico2w
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mqtt
    )
endif()

target_include_directories(bitcoin_miner_pico2w PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)


pico_enable_stdio_usb(bitcoin_miner_pico2w 1)
pico_enable_stdio_uart(bitcoin_miner_pico2w 0)


# create map/bin/hex file etc.
pico_add_extra_outputs(bitcoin_miner_pico2w)

# add url via pico_set_program_url

cmake_minimum_required(VERSION 3.22)

# Default target: Raspberry Pi Pico 2 (RP2350, ARM Cortex-M33).
# Override on the command line with -DPICO_BOARD=pico -DPICO_PLATFORM=rp2040
# to build for the original Pico (RP2040). The build-firmware.sh helper
# builds both variants in one go.
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2)
endif()
if(NOT DEFINED PICO_PLATFORM)
    set(PICO_PLATFORM rp2350-arm-s)
endif()

include(brain-sdk/pico_sdk_import.cmake)
project(brain-diagnostics C CXX ASM)

# Reserve top-of-flash sectors for Brain SDK calibration storage.
# MUST come before pico_sdk_init() so the linker excludes those sectors
# from the firmware image. Without this, flashing this UF2 can overwrite
# CV calibration that lives in the same flash region.
include(brain-sdk/cmake/brain-storage-reserve-flash.cmake)
brain_storage_configure_flash_reservation()

pico_sdk_init()

# Pull in only the Brain library, not the SDK's sandbox/examples/test trees.
add_subdirectory(brain-sdk/brain)

add_executable(brain-diagnostics
    main.cpp
    tests.cpp
)

target_compile_definitions(brain-diagnostics PRIVATE BRAIN_USE_ALL=1)
target_link_libraries(brain-diagnostics PRIVATE brain pico_stdlib)

# USB stdio so the device enumerates as a serial port; UART stdio off
# (the firmware is screen-free, but USB CDC is convenient for debugging).
pico_enable_stdio_usb(brain-diagnostics 1)
pico_enable_stdio_uart(brain-diagnostics 0)
pico_add_extra_outputs(brain-diagnostics)

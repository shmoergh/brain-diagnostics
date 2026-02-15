cmake_minimum_required(VERSION 3.22)

# Include Pico SDK (from brain-sdk subdirectory)
include(brain-sdk/pico_sdk_import.cmake)

project(brain-diagnostics C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Add brain-sdk which includes brain libraries
add_subdirectory(brain-sdk)

add_executable(brain-diagnostics
	main.cpp
	diagnostics.cpp
	pots_and_buttons.cpp
	inputs.cpp
	outputs.cpp
)

target_link_libraries(brain-diagnostics
	pico_stdlib
	brain-common
	brain-ui
	brain-io
  brain-utils)

pico_enable_stdio_usb(brain-diagnostics 1)
pico_enable_stdio_uart(brain-diagnostics 1)
pico_add_extra_outputs(brain-diagnostics)

# Minimal GPIO test executable
add_executable(gpio-test
	gpio-test.cpp
)

target_link_libraries(gpio-test
	pico_stdlib
	hardware_gpio
)

pico_enable_stdio_usb(gpio-test 1)
pico_enable_stdio_uart(gpio-test 1)
pico_add_extra_outputs(gpio-test)

cmake_minimum_required(VERSION 3.12)

set(ENV{PICO_SDK_PATH} "../pico-sdk")

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project(irq_lowpow.cpp C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()


add_executable(test
        irq_lowpow.cpp
        )

target_link_libraries(test
	pico_stdlib
)

pico_enable_stdio_usb(test 0)
pico_enable_stdio_uart(test 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(test)

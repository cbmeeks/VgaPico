cmake_minimum_required(VERSION 3.26)

# Set environment variables
# I prefer setting this here instead of in .bashrc or other OS-specific file.  Basically, you just need to make sure
# to set your PICO_SDK_PATH environment variable to the location of the Pico SDK on your system.
set(ENV{PICO_SDK_PATH} "/home/cbmeeks/sdk/pico/pico-sdk")

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Set the name of the project (as PROJECT_NAME) and C/C++ standards
project(VgaPico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
        vga_graphics.c
        demo.c
        bit_helper.h
)

# by default the header is generated into the build dir
# however, alternatively you can choose to generate it somewhere else (in this case in the source tree for check in)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/hsync.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/vsync.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/rgb.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# must match with executable name and source file names
target_sources(${PROJECT_NAME} PRIVATE demo.c vga_graphics.c)

# Link to libraries (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME}
        pico_multicore
        pico_stdlib
        hardware_adc
        hardware_dma
        hardware_irq
        hardware_pio
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)



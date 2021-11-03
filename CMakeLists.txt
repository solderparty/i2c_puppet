cmake_minimum_required(VERSION 3.13)

set(PICO_PLATFORM "rp2040")
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

include(3rdparty/pico-sdk/pico_sdk_init.cmake)

project(i2c_puppet)

pico_sdk_init()

add_subdirectory(app)

# binary info in flash
pico_set_program_name(i2c_puppet "I2C Puppet")
pico_set_program_version(i2c_puppet "0.1")

# printf targets
#pico_enable_stdio_usb(i2c_puppet 1)
pico_enable_stdio_uart(i2c_puppet 1)

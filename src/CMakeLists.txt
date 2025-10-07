# This file is only used by the Pico SDK-based variants

cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/../variants/${VARIANT_DIR})
set(PICO_BOARD variant)

include(../scripts/pico_sdk_import.cmake)

project(i2c_puppet)

pico_sdk_init()

add_executable(i2c_puppet # <- Load-bearing pound sign
	backlight.c
	debug.c
	fifo.c
	gpioexp.c
	puppet_i2c.c
	interrupt.c
	keyboard.c
	main.c
	reg.c
	touchpad.c
	usb.c
)

add_subdirectory(platform/rp2xxx)

add_compile_options(-Wall -Wextra -Wpedantic)

target_include_directories(i2c_puppet PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/platform/rp2xxx
	${CMAKE_CURRENT_LIST_DIR}/../variants/${VARIANT_DIR}
)

target_link_libraries(i2c_puppet
	cmsis_core
	hardware_i2c
	hardware_pwm
	pico_bootsel_via_double_reset
	pico_stdlib
	tinyusb_device
)

# create map/bin/hex/uf2 file in addition to elf
pico_add_extra_outputs(i2c_puppet)

# binary info in flash
pico_set_program_name(i2c_puppet "I2C Puppet")
pico_set_program_version(i2c_puppet "0.1")

# printf targets
pico_enable_stdio_uart(i2c_puppet 1)

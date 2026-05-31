# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(CMAKE_HOST_WIN32)
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
cmake_minimum_required(VERSION 3.13...3.27)
set(PICO_BOARD pico2 CACHE STRING "Board type")
set(PICO_PLATFORM rp2350 CACHE STRING "Platform")

# Initialize pico-sdk from submodule
# note: this must happen before project()
include(cmake_include/pico_sdk_import.cmake)

include(cmake_include/virtualenv_create.cmake)

project(pmemtest)

pico_sdk_init()


# Wrap FatFs into a proper target
add_library(fatfs
    ${PICO_SDK_PATH}/lib/tinyusb/lib/fatfs/source/ff.c
    ${PICO_SDK_PATH}/lib/tinyusb/lib/fatfs/source/ffsystem.c
    ${PICO_SDK_PATH}/lib/tinyusb/lib/fatfs/source/ffunicode.c
)
target_include_directories(fatfs PUBLIC
    ${PICO_SDK_PATH}/lib/tinyusb/lib/fatfs/source
)

# Define a target that runs the script
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/fat_image.c"
    COMMAND ${VENV_PYTHON} ${CMAKE_SOURCE_DIR}/scripts/generate_fat_image.py
        --input-dir=${CMAKE_CURRENT_LIST_DIR}/fat_files
        --output-file=${CMAKE_CURRENT_BINARY_DIR}/fat_image.h
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generate_fat_image.py
    COMMENT "Generating FAT filesystem image source files"
    VERBATIM
)

add_executable(
    pmemtest
    "${CMAKE_CURRENT_BINARY_DIR}/fat_image.c"
    )

# Help with overclock XIP abd flash writes.
target_compile_definitions(pmemtest PRIVATE 
    PICO_COPY_TO_RAM=1
    PICO_FLASH_SAFE_EXECUTE_PICO_SUPPORT_MULTICORE_LOCKOUT=0
    PICO_FLASH_SPI_CLKDIV=4
    PICO_FLASH_ASSUME_CORE1_SAFE=1 
)

# Optional: Force the compiler to not assume data is in Flash
target_compile_options(pmemtest PRIVATE -mno-pic-data-is-text-relative)

pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram4132.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram41128.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram41256.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram4b1r.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram1b1r.pio)

set(PICO_SDK_PATH "${USERHOME}/.pico-sdk/sdk/${sdkVersion}")

target_include_directories(pmemtest PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/shared_storage
	${CMAKE_CURRENT_LIST_DIR}/logging
#	${CMAKE_CURRENT_LIST_DIR}/gui
)

target_sources(pmemtest PRIVATE
    board.c
    config.c
    gui.c
    gui/st7789.c
    gui/fonts/sserif13.c;
    gui/fonts/sserif16.c;
    gui/fonts/sserif20.c;
    gui/fonts/widgets16.c;
    gui/icons/chip_icon.c
    gui/icons/check_icon.c
    gui/icons/error_icon.c
    gui/icons/drum_icon0.c
    gui/icons/drum_icon1.c
    gui/icons/drum_icon2.c
    gui/icons/drum_icon3.c
    gui/icons/warn_icon.c
    gui/icons/x_icon.c
    logging/ulog.c
    logging/logging.c
    mem_chip.c
    menu.c
    mem_tests.c
    pio_patcher.c
    queue.c
    pmemtest.c
    ram4027.c
    ram4108.c
    ram4116.c
    ram4132.c
    ram4164.c
    ram4408.c
    ram4416.c
    ram4464.c
    ram44256.c
    ram4816.c
    shared_storage/fat_little_flash.c
    shared_storage/pico_to_fat.c
	shared_storage/tusb_to_fat.c
    shared_storage/shared_storage.c
	shared_storage/tusb_descriptors.c
    xoroshiro64starstar.c
)

target_link_libraries(pmemtest PRIVATE
    pico_stdlib
    pico_flash
    hardware_flash
    pico_multicore
    hardware_pio
    hardware_spi
    tinyusb_device
    fatfs
)

pico_enable_stdio_usb(pmemtest 1)
pico_set_binary_type(pmemtest copy_to_ram)
pico_add_extra_outputs(pmemtest)

add_custom_command(
    TARGET pmemtest
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/build/pmemtest.uf2"
        "${CMAKE_CURRENT_LIST_DIR}/pmemtest.uf2"
    COMMENT "Copying uf2 to source directory"
)

#add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/submodules/pico-usb-flash-drive) 
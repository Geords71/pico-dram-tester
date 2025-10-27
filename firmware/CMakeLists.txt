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
cmake_minimum_required(VERSION 3.13...3.27)
set(PICO_BOARD pico2 CACHE STRING "Board type")
set(PICO_PLATFORM rp2350 CACHE STRING "Platform")

# Initialize pico-sdk from submodule
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pmemtest)

pico_sdk_init()

add_executable(pmemtest)

target_compile_definitions(pmemtest PRIVATE
	PLL_SYS_REFDIV=1
	PLL_SYS_VCO_FREQ_HZ=1500000000
	PLL_SYS_POSTDIV1=5
	PLL_SYS_POSTDIV2=1
)

#set(PIO_FILES  
    #ram4164
    #ram4116
    #ram4816
    #ram4132
    #ram41128
    #ram41256
    #ram_4bit
#)

#foreach (PIO_FILE IN LISTS PIO_FILES)
#    set(PIO_FILE ${PIO_FILE}.pio)
#    set(PIO_HEADER ${PIO_FILE}.pio.h)
#    set(PIO_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${PIO_FILE})
#    set(PIO_TARGET ${CMAKE_CURRENT_BINARY_DIR}/${PIO_HEADER})

#    add_custom_command(
#        OUTPUT ${PIO_HEADER}
#        COMMAND pioasm ${PIO_SOURCE} ${PIO_TARGET}
#        DEPENDS ${PIO_SOURCE}
#    )

#    add_custom_target(${PIO_FILE}_pio_header ALL
#        DEPENDS ${PIO_TARGET}
#        COMMENT "Generating ${PIO_HEADER} from ${PIO_FILE}"
#    )
#endforeach()
target_include_directories(pmemtest PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram4164.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram4116.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram4816.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram4132.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram41128.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram41256.pio)
pico_generate_pio_header(pmemtest ${CMAKE_CURRENT_LIST_DIR}/ram_4bit.pio)

target_sources(pmemtest PRIVATE pmemtest.c st7789.c gui.c pio_patcher.c xoroshiro64starstar.c constants.c)
target_link_libraries(pmemtest PRIVATE pico_stdlib pico_multicore hardware_pio hardware_spi)
pico_add_extra_outputs(pmemtest)

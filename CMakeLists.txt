cmake_minimum_required(VERSION 3.13)

# Board: "waveshare_rp2040_zero" (default, the board the bridge was tested on),
# "pico" or "pico_w".  RP2350 boards are untested.
set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")

# Dependencies: environment variables win, git submodules under lib/ are the fallback.
if (NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk)
endif()
if (NOT DEFINED PICO_PIO_USB_PATH AND NOT DEFINED ENV{PICO_PIO_USB_PATH})
    set(PICO_PIO_USB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/Pico-PIO-USB)
elseif (DEFINED ENV{PICO_PIO_USB_PATH} AND NOT DEFINED PICO_PIO_USB_PATH)
    set(PICO_PIO_USB_PATH $ENV{PICO_PIO_USB_PATH})
endif()
if (NOT EXISTS ${PICO_PIO_USB_PATH}/src/pio_usb.c)
    message(FATAL_ERROR "Pico-PIO-USB not found at ${PICO_PIO_USB_PATH}; run 'git submodule update --init'")
endif()

include(pico_sdk_import.cmake)

project(spacepilot_bridge C CXX ASM)

pico_sdk_init()

# Defined by TinyUSB's rp2040/family.cmake (pulled in by the SDK).  It creates the
# tinyusb_pico_pio_usb target from PICO_PIO_USB_PATH.
if (NOT DEFINED TOP)
    set(TOP ${PICO_TINYUSB_PATH})
endif()
check_and_add_pico_pio_usb_support()
if (NOT TARGET tinyusb_pico_pio_usb)
    message(FATAL_ERROR "tinyusb_pico_pio_usb target missing; is the pico-sdk TinyUSB submodule initialised?")
endif()

add_executable(spacepilot_bridge
    src/main.c
    src/bridge.c
    src/usb_descriptors.c
    src/usb_device.c
    src/usb_host.c
)

target_include_directories(spacepilot_bridge PRIVATE src)

# Extra -D flags, e.g. -DBRIDGE_EXTRA_DEFS="-DBRIDGE_DEBUG=2;-DBRIDGE_USB_PID=0xc632"
if (BRIDGE_EXTRA_DEFS)
    target_compile_options(spacepilot_bridge PRIVATE ${BRIDGE_EXTRA_DEFS})
endif()

target_compile_options(spacepilot_bridge PRIVATE -Wall -Wextra)

target_link_libraries(spacepilot_bridge
    pico_stdlib
    pico_stdio_uart
    pico_unique_id
    hardware_pio
    hardware_dma
    tinyusb_device
    tinyusb_host
    tinyusb_pico_pio_usb
)

# stdio goes to the UART on the pins from config.h, set up explicitly in main.c
pico_enable_stdio_usb(spacepilot_bridge 0)
pico_enable_stdio_uart(spacepilot_bridge 0)

# Run from RAM so that the PIO USB host is never stalled by XIP flash access.
pico_set_binary_type(spacepilot_bridge copy_to_ram)

pico_add_extra_outputs(spacepilot_bridge)

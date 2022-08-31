cmake_minimum_required(VERSION 3.13)

# remember to define PICO_SDK_PATH
include(pico_sdk_import.cmake)

project(lora_bridge C CXX ASM)
pico_sdk_init()

add_executable(lora_bridge
        main.c
        usb_command.c
        usb-descriptors.c
        e220.c)

target_include_directories(lora_bridge PUBLIC
        ./
        ${PICO_SDK_PATH}/lib/tinyusb/src)

target_link_libraries(lora_bridge
        pico_stdlib
        tinyusb_device)

pico_add_extra_outputs(lora_bridge)
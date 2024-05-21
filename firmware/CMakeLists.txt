cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(power_control C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if (TARGET tinyusb_device)

  add_executable(power_control
    power_control.c
  )

  target_link_libraries(power_control pico_stdlib pico_mem_ops)

  pico_enable_stdio_usb(power_control 0)
  pico_enable_stdio_uart(power_control 0)

  pico_add_extra_outputs(power_control)

elseif(PICO_ON_DEVICE)
   message(WARNING "not building because TinyUSB submodule is not initialized in the SDK")
endif()

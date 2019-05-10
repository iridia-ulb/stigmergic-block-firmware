cmake_minimum_required(VERSION 3.10)

# Use the AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)

set(AVR "atmega328p")
set(AVR_FREQ "8000000UL")
set(AVR_FIRMWARE_NAME firmware)

project("Stigmeric Block Firmware")

# Pass defines to compiler
add_definitions(
    -DF_CPU=${AVR_FREQ}
)
# mmcu MUST be passed to both the compiler and linker, this handle the linker
set(CMAKE_EXE_LINKER_FLAGS -mmcu=${AVR})

add_compile_options(
    -mmcu=${AVR}
    -std=c++11
    -Os
    -Wall
    -ffunction-sections
    -fdata-sections
    -fno-threadsafe-statics
)

set(FIRWMARE_HEADERS
   accelerometer_controller.h
   clock.h
   huart_controller.h
   interrupt.h
   led_controller.h
   nfc_controller.h
   port_controller.h
   system.h
   task_scheduler.h
   tuart_controller.h
   tw_controller.h
   utils/container.h
)

set(FIRMWARE_SOURCES
   ${FIRWMARE_HEADERS}
   accelerometer_controller.cxx
   clock.cxx
   huart_controller.cxx
   interrupt.cxx
   led_controller.cxx
   main.cxx
   nfc_controller.cxx
   port_controller.cxx
   system.cxx
   task_scheduler.cxx
   tuart_controller.cxx
   tw_controller.cxx
   utils/container.cxx
)

# Create one target
add_executable(${AVR_FIRMWARE_NAME} ${FIRMWARE_SOURCES})

# Rename the output to .elf as we will create multiple files
set_target_properties(${AVR_FIRMWARE_NAME} PROPERTIES OUTPUT_NAME ${AVR_FIRMWARE_NAME}.elf)

# Strip binary for upload
add_custom_target(strip ALL avr-strip ${AVR_FIRMWARE_NAME}.elf DEPENDS ${AVR_FIRMWARE_NAME})

# Transform binary into hex file
add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex ${AVR_FIRMWARE_NAME}.elf ${AVR_FIRMWARE_NAME}.hex DEPENDS strip)
add_custom_target(eeprom avr-objcopy -j .eeprom  --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex ${AVR_FIRMWARE_NAME}.elf ${AVR_FIRMWARE_NAME}.eep DEPENDS strip)

# Clean extra files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${AVR_FIRMWARE_NAME}.hex;${AVR_FIRMWARE_NAME}.eeprom;${AVR_FIRMWARE_NAME}.lst")


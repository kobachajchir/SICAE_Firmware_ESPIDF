cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(SICAE_IDF_Firmware)

# Ensure C++ standard is set
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List your component directories
list(APPEND EXTRA_COMPONENT_DIRS "components/jsoncpp" "components/esp_firebase" "components/arduino/esp32-arduino-lib-builder/components/arduino_tinyusb" "components/arduino/libraries/Arduino-IRremote-4.4.0")

# Generate SPIFFS image
spiffs_create_partition_image(spiffs data FLASH_IN_PROJECT)

# Optionally, remove these lines if ESP-IDF handles the compiler setup automatically
# These are usually set by ESP-IDF, so unless you have a specific reason, you may not need them.
set(CMAKE_C_COMPILER "xtensa-esp32-elf-gcc")
set(CMAKE_CXX_COMPILER "xtensa-esp32-elf-g++")

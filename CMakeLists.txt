cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SICAE_Firmware_ESPIDF)

# Ensure C++ standard is set
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List your component directories
list(APPEND EXTRA_COMPONENT_DIRS "components/components/esp_firebase" "components/components/jsoncpp")

set(CMAKE_C_COMPILER "xtensa-esp32-elf-gcc")
set(CMAKE_CXX_COMPILER "xtensa-esp32-elf-g++")
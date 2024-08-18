cmake_minimum_required(VERSION 3.5)
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.c)
list(APPEND EXTRA_COMPONENT_DIRS "../components/jsoncpp" "../components/esp_firebase" "../components/arduino/esp32-arduino-lib-builder/components/arduino_tinyusb" "../components/arduino/libraries/Arduino-IRremote-4.4.0"
  "../components/arduino/libraries/IRLib2/IRLib2")
idf_component_register(SRCS "devicesRelay_utils.c" "main.cpp" "wifi_utils.c" "lcd_utils.c" "webserver.c" "buttons.c" "firebaseFns.c" "spiffs.c" "nvs_utils.c" "httpMethods.c" "ntp.c" "IR_utils.c" "devicesRelay_utils.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_https_server esp-tls esp_wifi nvs_flash esp_http_client driver jsoncpp esp_firebase efuse spiffs arduino-esp32 arduino
                       EMBED_TXTFILES "servercert.pem" "serverkey.pem" "clientcert.pem")
# Link the C++ standard library
target_link_libraries(${COMPONENT_LIB} PRIVATE stdc++)

# Specify the C++ compiler explicitly
set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS "-x c++")

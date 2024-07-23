FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.c)
list(APPEND EXTRA_COMPONENT_DIRS "../components/components/esp_firebase" "../components/components/jsoncpp")
idf_component_register(SRCS "main.cpp" "wifi_utils.c" "lcd_utils.c" "webserver.c" "buttons.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_https_server esp-tls esp_wifi nvs_flash esp_http_client driver jsoncpp esp_firebase
                       EMBED_TXTFILES "servercert.pem" "serverkey.pem" "clientcert.pem")

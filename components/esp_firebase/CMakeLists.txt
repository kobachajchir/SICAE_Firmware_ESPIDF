
idf_component_register(SRCS "rtdb.cpp" "app.cpp"
                    INCLUDE_DIRS "." ".."
                    REQUIRES esp_http_client esp-tls jsoncpp
                    EMBED_TXTFILES gtsr1.pem)

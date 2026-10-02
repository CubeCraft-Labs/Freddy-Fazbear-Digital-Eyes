idf_component_register(SRCS "main.cpp" "ota_service.cpp" INCLUDE_DIRS "."
                       REQUIRES app_update esp_http_server esp_wifi esp_netif
                                nvs_flash mbedtls)

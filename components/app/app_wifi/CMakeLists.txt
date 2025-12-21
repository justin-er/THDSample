idf_component_register(SRCS "app_wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES config esp_netif esp_wifi freertos esp_event nvs_flash app_nvs http_server)
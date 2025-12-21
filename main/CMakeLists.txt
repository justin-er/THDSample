idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES config humidity_indicator led_controller app_wifi app_nvs http_server)
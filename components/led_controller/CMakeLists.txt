idf_component_register(SRCS "led_controller.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver)

set(CONFIG_DHT_READER_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/Kconfig)
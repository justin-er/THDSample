idf_component_register(SRCS "dht_reader.c"
                    INCLUDE_DIRS "include")

set(CONFIG_DHT_READER_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/Kconfig)
idf_component_register(
    SRCS "app_coordinator.c"
    INCLUDE_DIRS "include"
    REQUIRES dht_reader app_nvs ota_update esp_timer
)


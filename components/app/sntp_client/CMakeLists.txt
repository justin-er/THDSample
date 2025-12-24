idf_component_register(
    SRCS "sntp_client.c"
    INCLUDE_DIRS "include"
    REQUIRES lwip esp_netif config
)


idf_component_register(
    SRCS "http_server.c"
    INCLUDE_DIRS "include"
    REQUIRES app_coordinator app_wifi esp_http_server cjson
    EMBED_FILES
        "${CMAKE_SOURCE_DIR}/main/webpage/index.html"
        "${CMAKE_SOURCE_DIR}/main/webpage/app.css"
        "${CMAKE_SOURCE_DIR}/main/webpage/app.js"
        "${CMAKE_SOURCE_DIR}/main/webpage/jquery-3.3.1.min.js"
        "${CMAKE_SOURCE_DIR}/main/webpage/favicon.ico"
)

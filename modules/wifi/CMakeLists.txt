set(SOURCES wifi.c)
set(DEPENDENCIES freertos esp_common nvs_flash Credentials)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . 
                        REQUIRES ${DEPENDENCIES})

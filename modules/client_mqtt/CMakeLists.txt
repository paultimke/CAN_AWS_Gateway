set(SOURCES client_mqtt.cpp)
set(DEPENDENCIES freertos esp_common mqtt)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES ${DEPENDENCIES})
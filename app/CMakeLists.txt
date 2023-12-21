set(SOURCES main.cpp application.cpp)
set(DEPENDENCIES freertos esp_common client_mqtt wifi)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES ${DEPENDENCIES})
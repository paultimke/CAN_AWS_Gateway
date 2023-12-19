set(SOURCES main.cpp application.cpp)
set(DEPENDENCIES freertos esp_common driver nvs_flash mqtt protocol_examples_common)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES ${DEPENDENCIES})
set(SOURCES main.cpp application.cpp)
set(DEPENDENCIES freertos can_bus wifi aws)
set(INCLUDES "." "${PROJECT_DIR}/common_config")

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS ${INCLUDES}
                        REQUIRES ${DEPENDENCIES})
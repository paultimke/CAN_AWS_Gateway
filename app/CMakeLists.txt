set(SOURCES main.cpp application.cpp)
set(DEPENDENCIES freertos wifi aws can)
set(INCLUDES "." "${PROJECT_DIR}/common_config")

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS ${INCLUDES}
                        REQUIRES ${DEPENDENCIES})
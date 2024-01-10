set(SOURCES can_bus.c)
set(DEPENDENCIES driver app mcp2515 spi)
set(INCLUDES "." "${PROJECT_DIR}/common_config")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES ${DEPENDENCIES}
)

idf_component_register(
    SRCS "mcp2515.c"
    INCLUDE_DIRS "." "${PROJECT_DIR}/common_config"
    REQUIRES driver
)

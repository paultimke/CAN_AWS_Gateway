idf_component_register(
    SRCS "spi.c"
    INCLUDE_DIRS "." "${PROJECT_DIR}/common_config"
    REQUIRES driver
)

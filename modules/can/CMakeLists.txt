idf_component_register(
    SRCS "can.c"
    INCLUDE_DIRS "." "${PROJECT_DIR}/common_config" 
    REQUIRES driver mcp2515 app
)

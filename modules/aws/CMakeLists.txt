set(SOURCES aws_iot.c)
set(DEPENDENCIES app freertos esp_common nvs_flash fatfs esp-aws-iot)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . "${PROJECT_DIR}/common_config"
                        REQUIRES ${DEPENDENCIES})

target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_SOURCE_DIR}/certs/aws_root_ca_pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_SOURCE_DIR}/certs/certificate_pem_crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_SOURCE_DIR}/certs/private_pem_key" TEXT)
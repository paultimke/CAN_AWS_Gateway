// ***************************************************** //
/// @file rtos_config.h
/// @brief RTOS config file
/// @date 2023-12-23
// ***************************************************** //

#ifndef _RTOS_CONFIG_H_
#define _RTOS_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// --------------------------------------------------
// Constants 
// --------------------------------------------------
#define CLI_TASK_STACK_SIZE         (1024 * 2)
#define APP_TASK_STACK_SIZE         (1024 * 3)
#define AWS_TASK_STACK_SIZE         (1024 * 9)

#define CLI_TASK_PRIORITY           (configMAX_PRIORITIES - 8)
#define APP_TASK_PRIORITY           (configMAX_PRIORITIES - 7)
#define AWS_TASK_PRIORITY           (configMAX_PRIORITIES - 20)

#ifdef __cplusplus
}
#endif

#endif // _RTOS_CONFIG_H_
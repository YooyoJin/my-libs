/**
 * @file    FreeRTOSConfig.h
 * @brief   FreeRTOS配置(HC32F460, 200MHz, Cortex-M4F)
 * @note    此文件是FreeRTOS的配置入口, 允许修改(vendor规则的例外)
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// ----------------------------------------------------------
// 基础配置
// ----------------------------------------------------------
#define configUSE_PREEMPTION                    1      // 抢占式调度
#define configUSE_TIME_SLICING                  1      // 同优先级时间片轮转
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0      // 不用CLZ指令选任务(兼容性)
#define configUSE_TICKLESS_IDLE                 0      // 不用tickless低功耗
#define configCPU_CLOCK_HZ                      (200000000UL) // 主频200MHz
#define configTICK_RATE_HZ                      ((TickType_t)1000) // 1ms节拍
#define configMAX_PRIORITIES                    7      // 优先级数(0最高,6最低)
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)  // 空闲任务栈(字)
#define configMAX_TASK_NAME_LEN                 16     // 任务名最大长度
#define configUSE_16_BIT_TICKS                  0      // 32位tick计数
#define configIDLE_SHOULD_YIELD                 1      // 空闲任务让出CPU

// ----------------------------------------------------------
// 内存配置(heap_4: 支持free+合并碎片)
// ----------------------------------------------------------
#define configTOTAL_HEAP_SIZE                   ((size_t)32*1024) // FreeRTOS堆32KB
#define configAPPLICATION_ALLOCATED_HEAP        0      // 用FreeRTOS自带的堆

// ----------------------------------------------------------
// 任务功能
// ----------------------------------------------------------
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_NEWLIB_REENTRANT              0      // 不用newlib重入(轻量)

// ----------------------------------------------------------
// 软件定时器
// ----------------------------------------------------------
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            256

// ----------------------------------------------------------
// 中断优先级(Cortex-M4, 4位优先级: 0=最高, 15=最低)
// ----------------------------------------------------------
// 优先级0~4: 不能调用FreeRTOS API的高速中断
// 优先级5~15: 可以调用FreeRTOS API(FromISR版本)
#define configKERNEL_INTERRUPT_PRIORITY         15 << 4   // SysTick/PendSV=最低优先级
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     5 << 4   // 可调用API的最高优先级

// ----------------------------------------------------------
// 可选API
// ----------------------------------------------------------
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_eTaskGetState                   1

// ----------------------------------------------------------
// 钩子函数
// ----------------------------------------------------------
#define configUSE_IDLE_HOOK                     0      // 空闲任务钩子(不用)
#define configUSE_TICK_HOOK                     0      // tick钩子(不用)
#define configUSE_MALLOC_FAILED_HOOK            1      // malloc失败钩子(建议开!)
#define configCHECK_FOR_STACK_OVERFLOW          2      // 栈溢出检测(方法2)

// ----------------------------------------------------------
// 运行时统计(调试期开,量产关)
// ----------------------------------------------------------
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1      // 供uxTaskGetSystemState用
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

// ----------------------------------------------------------
// 断言
// ----------------------------------------------------------
#define configASSERT(x)                         \
    do {                                           \
        if ((x) == 0) {                            \
            taskDISABLE_INTERRUPTS();              \
            for (;;);                              \
        }                                          \
    } while (0)

// ----------------------------------------------------------
// Cortex-M特有: 让FreeRTOS接管SysTick/PendSV/SVC
// ----------------------------------------------------------
#define configPRIO_BITS                         4      // Cortex-M4用4位优先级
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#endif // FREERTOS_CONFIG_H

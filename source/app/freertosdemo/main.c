/**
 * @file    main.c
 * @brief   FreeRTOS版演示应用(HC32F460, 组装根)
 *
 * 与前后台版(hc32f460demo)的区别:
 *   - 无while(1)主循环 → vTaskStartScheduler()接管
 *   - 无apptimer/SysTick → FreeRTOS自带
 *   - 每个功能是一个独立任务,各有自己的栈和优先级
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hc32_ll.h"
#include "log.h"
#include "rttlog.h"
#include "bspclk.h"
#include "bspled.h"

/**
 * @brief  FreeRTOS栈溢出钩子(configCHECK_FOR_STACK_OVERFLOW=2时被调用)
 * @note   任务栈不够用时会到这里——增大栈或检查递归
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    LOG_ERROR("Stack overflow in: %s", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/**
 * @brief  FreeRTOS堆分配失败钩子(configUSE_MALLOC_FAILED_HOOK=1时被调用)
 * @note   FreeRTOS堆不够——增大configTOTAL_HEAP_SIZE
 */
void vApplicationMallocFailedHook(void)
{
    LOG_ERROR("FreeRTOS heap exhausted (%u free)", (unsigned)xPortGetFreeHeapSize());
    taskDISABLE_INTERRUPTS();
    for (;;);
}

// 静态任务句柄
static TaskHandle_t s_xTaskLED;
static TaskHandle_t s_xTaskLog;

/**
 * @brief  LED任务: 每秒翻转LED+打印计数
 */
static void TaskLED(void *pvParameters)
{
    uint32_t u32Count = 0;

    (void)pvParameters;
    while (1)
    {
        BspLedToggle();
        LOG_INFO("[LED] tick %u", (unsigned)++u32Count);
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1秒,阻塞延时(让出CPU)
    }
}

/**
 * @brief  日志任务: 每3秒打印一次运行统计(证明多任务并行)
 */
static void TaskLog(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        LOG_INFO("[LOG] free heap: %u bytes", (unsigned)xPortGetFreeHeapSize());
        LOG_INFO("[LOG] min stack LED: %u words",
                 (unsigned)uxTaskGetStackHighWaterMark(s_xTaskLED));
        vTaskDelay(pdMS_TO_TICKS(3000));  // 3秒
    }
}

/**
 * @brief  应用入口(FreeRTOS版)
 */
int main(void)
{
    // 硬件初始化(与前后台版相同的port层)
    LL_PERIPH_WE(LL_PERIPH_ALL);
    RttLogInit();       // log模块+RTT后端
    BspClkInit();       // 时钟(200MHz)——注意: 不初始化SysTick!
    BspLedInit();       // LED
    LL_PERIPH_WP(LL_PERIPH_ALL);

    LOG_INFO("FreeRTOS demo start");

    // 创建任务
    xTaskCreate(TaskLED,  "LED",  256, NULL, 2, &s_xTaskLED);
    xTaskCreate(TaskLog,  "LOG",  256, NULL, 1, &s_xTaskLog);

    // 启动调度器(永不返回)
    LOG_INFO("Starting scheduler...");
    vTaskStartScheduler();

    // 不应该到这里
    LOG_ERROR("Scheduler failed to start!");
    while (1);
}

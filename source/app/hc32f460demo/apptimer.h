/**
 * @file    apptimer.h
 * @brief   前后台版时间片调度+SysTick中断(归app层所有)
 * @note    SysTick是调度资源——前后台版在此拥有, FreeRTOS版由RTOS port接管
 */
#ifndef APPTIMER_H
#define APPTIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "log.h"

/**
 * @brief  测量函数运行时间并打印(DEBUG级别)
 * @param  func      要测量的函数调用
 * @param  funcName  函数名称字符串
 */
#define MEASURE_FUNC_TIME_MS(func, funcName)         \
    do {                                             \
        uint32_t u32Start = AppTimerGetTick();       \
        func;                                        \
        uint32_t u32Exec = AppTimerGetTick() - u32Start; \
        LOG_DEBUG("[%s] execution time: %u ms", funcName, u32Exec); \
    } while (0)

/** @brief 时间片调度初始化(向BSP注册SysTick回调) */
void AppTimerInit(void);

/** @brief 时间片任务调度, 主循环里反复调用 */
void AppTimerTaskRun(void);

/** @brief 获取系统毫秒节拍 */
uint32_t AppTimerGetTick(void);

/** @brief SysTick回调(1ms中断上下文, 只置标志/计数) */
void AppTimerSysTickCallback(void);

#ifdef __cplusplus
}
#endif

#endif // APPTIMER_H

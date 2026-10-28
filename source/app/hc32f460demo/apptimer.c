/**
 * @file    apptimer.c
 * @brief   应用层时间片调度实现
 * @note    移植自My-OTA工程app_timer.c, 逻辑未改动
 */
#include <stdint.h>
#include <stdbool.h>
#include "apptimer.h"
#include "appexcute.h"
#include "log.h"
#include "hc32_ll_utility.h"

typedef void (*AppTaskFn)(void);

// 1s任务表: 为了提高实时性, 1s任务拆分成100个10ms时间片分时执行
// (槽位N的任务在每秒的第N×10ms执行; 槽位即执行时刻, 按需错峰登记)
static const AppTaskFn g_apfnTimer1sTaskList[] = {
    AppExcuteHeartbeatTask,   // 槽位0: 心跳日志(见appexcute.c)
};

static uint32_t      g_u32SystemTickMs  = 0;  // 系统毫秒节拍
static volatile bool g_b10msFlag        = false;
static uint16_t      g_u16Tick10msCnt   = 0;
static uint8_t       g_u8TimeSliceId    = 0;  // 1s任务的时间片编号(0~99)

/**
 * @brief  分时1s任务: 每10ms执行任务表中的一个槽位
 */
static void AppTimer1sTask(void)
{
    if (++g_u8TimeSliceId >= 100u)
    {
        g_u8TimeSliceId = 0;
    }

    if (g_u8TimeSliceId < (sizeof(g_apfnTimer1sTaskList) / sizeof(g_apfnTimer1sTaskList[0])))
    {
        g_apfnTimer1sTaskList[g_u8TimeSliceId]();
    }
}

/**
 * @brief  时间片调度初始化: 初始化SysTick(1ms)——前后台版在此拥有SysTick
 * @note   FreeRTOS版不需要此函数(RTOS port自带SysTick_Handler)
 */
void AppTimerInit(void)
{
    if (LL_OK != SysTick_Init(1000U))
    {
        LOG_ERROR("SysTick init FAILED");
    }
}

/**
 * @brief  SysTick中断服务函数(1ms中断上下文)
 * @note   前后台版在此定义——与FreeRTOS版互斥(不能同时存在)
 */
void SysTick_Handler(void)
{
    SysTick_IncTick();

    g_u16Tick10msCnt++;
    if (g_u16Tick10msCnt >= 10u)
    {
        g_b10msFlag      = true;
        g_u16Tick10msCnt = 0;
    }
    g_u32SystemTickMs += 1u;
}

/** @brief 时间片任务调度: 10ms标志到了执行一个槽位 */
void AppTimerTaskRun(void)
{
    if (g_b10msFlag)
    {
        g_b10msFlag = false;
        AppTimer1sTask();
        // 实时性要求较高的任务在此追加
    }
}

/** @brief 获取系统毫秒节拍 */
uint32_t AppTimerGetTick(void)
{
    return g_u32SystemTickMs;
}

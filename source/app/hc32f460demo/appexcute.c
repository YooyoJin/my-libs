/**
 * @file    appexcute.c
 * @brief   应用业务任务集合实现
 */
#include <stdint.h>
#include "appexcute.h"
#include "log.h"

static uint32_t g_u32RunSec = 0;  // 运行秒数

/**
 * @brief 1s心跳任务: 打印运行秒数
 * @note  兼作SysTick存活探针——本任务由SysTick链路驱动,
 *        若心跳停止而主循环仍活着, 说明SysTick/调度链路断了
 */
void AppExcuteHeartbeatTask(void)
{
    g_u32RunSec++;
    LOG_INFO("Heartbeat %us", g_u32RunSec);
}
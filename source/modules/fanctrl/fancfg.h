/**
 * @file    fancfg.h
 * @brief   fanctrl默认参数预设(工厂函数FanCfgDefault)
 *
 * 默认值移植自Y2C022S2工程Vars.c当前生效值
 * (CARRIER_FREQ==2000, 非WLZKSP_170_FAN分支), 与原工程对应关系:
 *   - u16OutAtMaxRpm(1500)   <-- uhFanLimitL  (Par.h: PG_TRIG_LOWER)
 *   - u16OutAtMinRpm(7000)   <-- uhFanLimitH  (Par.h: PG_TRIG50_UPPER)
 *   - u16OutStartup (6700)   <-- uhFanStart   (Par.h: PG_START_TIME)
 *   - s16PidKp/Ki/Kd (5/1/1) <-- uhKp/uhKi/uhKd
 *   - u8PulsePerRev(12)      <-- PG_FEED_BACK_PLS
 *   - u8StartupHoldSec(20)   <-- DC_Fan.c: gucFan1OnTime>=20
 *     (注意: 原注释误标3s, 按1s任务计数实为20s)
 */
#ifndef FANCFG_H
#define FANCFG_H

#include "fanctrl.h"

/**
 * @brief   默认配置工厂:返回一份默认FanCfg
 * @note    用法: FanCfg stCfg = FanCfgDefault();
 *          板级调参(继承默认后微调):
 *          stCfg.u8StartupHoldSec = 5;
 */
static inline FanCfg FanCfgDefault(void)
{
    FanCfg stCfg = {
        // 输出量纲(反向刻度)
        .u16OutAtMaxRpm   = 1500u,
        .u16OutAtMinRpm   = 7000u,
        .u16OutStartup    = 6700u,
        .u16OutSearchStep = 500u,

        // 增量式PID
        .s16PidKp = 5,
        .s16PidKi = 1,
        .s16PidKd = 1,

        // 反馈测速(512us滴答, 12脉冲/转=24边沿/转, 每转测一次)
        .u16TickUs      = 512u,
        .u8PulsePerRev  = 12u,
        .u8PulseDivide  = 24u,
        .u8PidDivide    = 1u,

        // 转速限幅(RPM)
        .u16RpmMin = 200u,
        .u16RpmMax = 1500u,

        // 启动/保护时序(秒)
        .u8StartupHoldSec   = 20u,
        .u8NoFeedJudgeSec   = 10u,
        .u8SearchStepSec    = 2u,
        .u8SearchTimeoutSec = 30u,
        .u8SearchRetryMax   = 2u,
        .u8RetryStopSec     = 5u,
    };
    return stCfg;
}

#endif // FANCFG_H
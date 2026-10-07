/*============================================================================
 *  fancfg.h
 *
 *  默认参数预设 —— 移植自 Y2C022S2 工程 Vars.c 当前生效值
 *  (CARRIER_FREQ==2000, 非 WLZKSP_170_FAN 分支), 与原工程对应关系:
 *
 *    u16OutAtMaxRpm(1500)   <-- uhFanLimitL  (Par.h: PG_TRIG_LOWER)
 *    u16OutAtMinRpm(7000)   <-- uhFanLimitH  (Par.h: PG_TRIG50_UPPER)
 *    u16OutStartup (6700)   <-- uhFanStart   (Par.h: PG_START_TIME)
 *    s16PidKp/Ki/Kd (5/1/1) <-- uhKp/uhKi/uhKd
 *    u8PulsePerRev(12)      <-- PG_FEED_BACK_PLS
 *    u8StartupHoldSec(20)   <-- DC_Fan.c: gucFan1OnTime>=20
 *                              (注意: 原注释误标 3s, 按 1s 任务计数实为 20s)
 *==========================================================================*/
#ifndef FANCFG_H
#define FANCFG_H

#include "fanctrl.h"

/* 用法示例:
 *   static const FanCfg kstFanCfg = FAN_CFG_DEFAULT;
 *   FanCtrlInit(&kstFan1, &kstFanCfg, &kstFanPort1);
 *
 *   板级调参: 复制一份宏改名后按需修改数值, 不要直接改本预设。
 */
#define FAN_CFG_DEFAULT                                  \
{                                                       \
    /* 输出量纲 (反向刻度) */                             \
    .u16OutAtMaxRpm   = 1500u,                          \
    .u16OutAtMinRpm   = 7000u,                          \
    .u16OutStartup    = 6700u,                          \
    .u16OutSearchStep = 500u,                           \
                                                        \
    /* 增量式 PID */                                      \
    .s16PidKp = 5,                                      \
    .s16PidKi = 1,                                      \
    .s16PidKd = 1,                                      \
                                                        \
    /* 反馈测速 (512us 滴答, 12 脉冲/转, 每转测一次) */     \
    .u16TickUs      = 512u,                             \
    .u8PulsePerRev  = 12u,                              \
    .u8PulseDivide  = 12u,                              \
    .u8PidDivide    = 1u,                               \
                                                        \
    /* 转速限幅 (RPM) */                                   \
    .u16RpmMin = 200u,                                  \
    .u16RpmMax = 1500u,                                 \
                                                        \
    /* 启动/保护时序 (秒) */                               \
    .u8StartupHoldSec   = 20u,                          \
    .u8NoFeedJudgeSec   = 10u,                          \
    .u8SearchStepSec    = 2u,                           \
    .u8SearchTimeoutSec = 30u,                          \
    .u8SearchRetryMax   = 2u,                           \
    .u8RetryStopSec     = 5u,                           \
}

#endif /* FANCFG_H */

/*---------------------------- End Of fancfg.h ----------------------------*/
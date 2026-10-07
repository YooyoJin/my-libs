/*============================================================================
 *  fanctrl.h
 *
 *  PG/DC 风机转速闭环控制库 —— 纯逻辑层（零芯片依赖, 可移植）
 *
 *  功能:
 *    - PG 反馈脉冲测速 (RPM)
 *    - 增量式 PID 转速闭环 (输出为反向刻度的 PWM 控制量)
 *    - 启动保持 (默认 20s 锁定输出) -> 闭环切换
 *    - 丢反馈三级保护: 步进搜索 -> 停转重试 -> 故障锁定
 *    - 双风机: 实例化设计, 一份代码驱动多个 FanCtrl 实例
 *
 *  命名规范:
 *    函数/API : 大驼峰   FanCtrlInit
 *    变量     : 小驼峰 + 类型前缀 b/u8/u16/u32/s16/s32/e/pst
 *
 *  移植: 只需实现 FanPort 中的两个硬件钩子 (详见 readme.md 移植指南)
 *
 *  调用模型:
 *    FanCtrlTickFast()   挂 512us 级定时中断 (测速 + PID)
 *    FanCtrlTickSecond() 挂 1s 周期任务   (状态机时序)
 *
 *  版本: v0.1.0
 *==========================================================================*/
#ifndef FANCTRL_H
#define FANCTRL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------- * 状态机
 *------------------------------------------------------------------------*/
typedef enum
{
    FanStateIdle = 0,    /* 停机: 输出关闭 */
    FanStateStartupHold, /* 启动保持期: 输出锁定为 u16OutStartup (默认 20s) */
    FanStateRunning,     /* 闭环: PID 调节, 输出钳位 [u16OutAtMaxRpm, u16OutAtMinRpm] */
    FanStateSearch,      /* 丢反馈: 输出按步进推向最高转速端搜索脉冲 */
    FanStateFault        /* 故障锁定: 输出关闭, 需 FanCtrlClearFault() 解锁 */
} FanState;

/*-------------------------------------------------------------------------- * 硬件钩子 (移植层实现)
 *------------------------------------------------------------------------*/
typedef void (*FanWriteDutyFn)(uint16_t u16DutyPermille); /* 占空比输出, 0~1000 千分比 */
typedef bool (*FanReadLevelFn)(void);                     /* PG 反馈电平读取           */

typedef struct
{
    FanWriteDutyFn FanWriteDuty; /* 必须实现; 0 千分比 = 关闭输出 */
    FanReadLevelFn FanReadLevel; /* 必须实现 */
} FanPort;

/*-------------------------------------------------------------------------- * 配置参数 (实例生命周期内只读; 默认值见 fancfg.h)
 *------------------------------------------------------------------------*/
typedef struct
{
    /* ---- 输出量纲 (反向刻度: 数值越大 -> 占空比越小 -> 转速越低) ---- */
    uint16_t u16OutAtMaxRpm;   /* 最高转速对应输出值, PID 输出下限  默认 1500 */
    uint16_t u16OutAtMinRpm;   /* 最低转速对应输出值, PID 输出上限  默认 7000 */
    uint16_t u16OutStartup;    /* 启动保持期输出值                  默认 6700 */
    uint16_t u16OutSearchStep; /* 丢反馈搜索时每步削减量            默认  500 */

    /* ---- 增量式 PID 系数 ---- */
    int16_t  s16PidKp;         /* 默认 5 */
    int16_t  s16PidKi;         /* 默认 1 */
    int16_t  s16PidKd;         /* 默认 1 */

    /* ---- 反馈测速 ---- */
    uint16_t u16TickUs;        /* FanCtrlTickFast 调用周期, us      默认  512 */
    uint8_t  u8PulsePerRev;    /* 每转反馈脉冲数                    默认   12 */
    uint8_t  u8PulseDivide;    /* 每多少个电平边沿计一次转速(=每转脉冲数x2) 默认24 */
    uint8_t  u8PidDivide;      /* 每多少次测速执行一次 PID          默认    1 */

    /* ---- 转速限幅 ---- */
    uint16_t u16RpmMin;        /* 目标转速下限 (RPM)  默认  200 */
    uint16_t u16RpmMax;        /* 目标转速上限 (RPM)  默认 1500 */

    /* ---- 启动/保护时序 (单位: 秒, 由 FanCtrlTickSecond 驱动) ---- */
    uint8_t  u8StartupHoldSec;   /* 启动保持时长         默认 20 */
    uint8_t  u8NoFeedJudgeSec;   /* 判丢反馈时限         默认 10 */
    uint8_t  u8SearchStepSec;    /* 搜索步进周期         默认  2 */
    uint8_t  u8SearchTimeoutSec; /* 单轮搜索超时         默认 30 */
    uint8_t  u8SearchRetryMax;   /* 搜索轮数上限         默认  2 */
    uint8_t  u8RetryStopSec;     /* 轮间停转时长         默认  5 */
} FanCfg;

/*-------------------------------------------------------------------------- * 运行实例 (调用者分配; 双风机: FanCtrl fan[2] 各绑一个 FanPort)
 *------------------------------------------------------------------------*/
typedef struct
{
    /* 绑定的配置与硬件端口 (Init 后不可更换) */
    const FanCfg  *pstCfg;
    const FanPort *pstPort;

    /* 状态机 */
    FanState eState;
    bool     bEnabled;       /* 总使能 */
    bool     bFeedAlive;     /* 本秒内是否收到有效反馈 */
    bool     bRetryStopping; /* 搜索轮间的停转间歇标志 */

    /* 目标与工况 */
    uint16_t u16TargetRpm;   /* SV: 目标转速 (RPM) */
    uint16_t u16RpmPv;       /* PV: 滤波后实测转速 (RPM) */
    uint16_t u16Output;      /* 当前输出 (反向刻度) */

    /* 反馈测速 */
    bool     bLastLevel;     /* 上次反馈电平 (边沿检测) */
    uint8_t  u8PulseRemain;  /* 脉冲组分频剩余计数 */
    uint32_t u32SpeedTicks;  /* 距上次脉冲组的滴答计数 */
    uint32_t u32NoFeedTickLimit; /* 折算成滴答数的无反馈判限 */

    /* PID (预计算系数与历史误差) */
    int32_t  s32Ka;
    int32_t  s32Kb;
    int32_t  s32Kc;
    int16_t  s16ErrPrev;
    int16_t  s16ErrLast;
    uint8_t  u8PidCount;

    /* 秒级时序计数 */
    uint8_t  u8StartupSec;
    uint8_t  u8NoFeedSec;
    uint8_t  u8SearchSec;
    uint8_t  u8RetryStopSec;
    uint8_t  u8FailCount;
} FanCtrl;

/*-------------------------------------------------------------------------- * API
 *------------------------------------------------------------------------*/
void     FanCtrlInit(FanCtrl *pstFan, const FanCfg *pstCfg, const FanPort *pstPort);
void     FanCtrlStart(FanCtrl *pstFan);                         /* 启动: 进入启动保持期 */
void     FanCtrlStop(FanCtrl *pstFan);                          /* 停机: 输出关闭       */
bool     FanCtrlSetTargetRpm(FanCtrl *pstFan, uint16_t u16Rpm); /* 设目标(自动钳位)    */
void     FanCtrlTickFast(FanCtrl *pstFan);                      /* 512us 中断上下文    */
void     FanCtrlTickSecond(FanCtrl *pstFan);                    /* 1s 任务上下文       */
uint16_t FanCtrlGetRpm(const FanCtrl *pstFan);                  /* 实测转速 PV (RPM)   */
uint16_t FanCtrlGetOutput(const FanCtrl *pstFan);               /* 当前输出(反向刻度)  */
FanState FanCtrlGetState(const FanCtrl *pstFan);                /* 当前状态            */
bool     FanCtrlIsFault(const FanCtrl *pstFan);                 /* 故障查询            */
void     FanCtrlClearFault(FanCtrl *pstFan);                    /* 故障解锁 (回 Idle)  */

#ifdef __cplusplus
}
#endif

#endif /* FANCTRL_H */

/*---------------------------- End Of fanctrl.h ---------------------------*/
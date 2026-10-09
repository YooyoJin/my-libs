/**
 * @file    fanctrl.h
 * @brief   PG/DC风机转速闭环控制库——纯逻辑层(零芯片依赖,可移植)
 *
 * 功能:
 *   - PG反馈脉冲测速(RPM)
 *   - 增量式PID转速闭环(输出为反向刻度的PWM控制量)
 *   - 启动保持(默认20s锁定输出)后切闭环
 *   - 丢反馈三级保护:步进搜索→停转重试→故障锁定
 *   - 双风机:实例化设计,一份代码驱动多个FanCtrl实例
 *
 * 命名规范:
 *   - 函数/API:大驼峰  FanCtrlInit
 *   - 变量:小驼峰+类型前缀 b/u8/u16/u32/s16/s32/e/pst
 *
 * 移植:只需实现FanPort中的两个硬件钩子(详见readme.md移植指南)
 *
 * 调用模型:
 *   - FanCtrlTickFast()  挂512us级定时中断(测速+PID)
 *   - FanCtrlTickSecond()挂1s周期任务(状态机时序)
 *
 * @version v0.1.0
 */
#ifndef FANCTRL_H
#define FANCTRL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// 状态机
// ----------------------------------------------------------------------------
typedef enum
{
    FanStateIdle = 0,    // 停机:输出关闭
    FanStateStartupHold, // 启动保持期:输出锁定为u16OutStartup(默认20s)
    FanStateRunning,     // 闭环:PID调节,输出钳位[u16OutAtMaxRpm, u16OutAtMinRpm]
    FanStateSearch,      // 丢反馈:输出按步进推向最高转速端搜索脉冲
    FanStateFault        // 故障锁定:输出关闭,需FanCtrlClearFault()解锁
} FanState;

// ----------------------------------------------------------------------------
// 硬件钩子(移植层实现)
// ----------------------------------------------------------------------------
typedef void (*FanWriteDutyFn)(uint16_t u16DutyPermille); // 占空比输出,0~1000千分比
typedef bool (*FanReadLevelFn)(void);                     // PG反馈电平读取

typedef struct
{
    FanWriteDutyFn FanWriteDuty; // 必须实现;0千分比=关闭输出
    FanReadLevelFn FanReadLevel; // 必须实现
} FanPort;

// ----------------------------------------------------------------------------
// 配置参数(实例生命周期内只读;默认值见fancfg.h)
// ----------------------------------------------------------------------------
typedef struct
{
    // ---- 输出量纲(反向刻度:数值越大→占空比越小→转速越低) ----
    uint16_t u16OutAtMaxRpm;   // 最高转速对应输出值,PID输出下限  默认1500
    uint16_t u16OutAtMinRpm;   // 最低转速对应输出值,PID输出上限  默认7000
    uint16_t u16OutStartup;    // 启动保持期输出值  默认6700
    uint16_t u16OutSearchStep; // 丢反馈搜索时每步削减量  默认 500

    // ---- 增量式PID系数 ----
    int16_t  s16PidKp;         // 默认5
    int16_t  s16PidKi;         // 默认1
    int16_t  s16PidKd;         // 默认1

    // ---- 反馈测速 ----
    uint16_t u16TickUs;        // FanCtrlTickFast调用周期,us  默认512
    uint8_t  u8PulsePerRev;    // 每转反馈脉冲数  默认12
    uint8_t  u8PulseDivide;    // 每多少个电平边沿计一次转速(=每转脉冲数x2)  默认24
    uint8_t  u8PidDivide;      // 每多少次测速执行一次PID  默认1

    // ---- 转速限幅 ----
    uint16_t u16RpmMin;        // 目标转速下限(RPM)  默认200
    uint16_t u16RpmMax;        // 目标转速上限(RPM)  默认1500

    // ---- 启动/保护时序(单位:秒,由FanCtrlTickSecond驱动) ----
    uint8_t  u8StartupHoldSec;   // 启动保持时长  默认20
    uint8_t  u8NoFeedJudgeSec;   // 判丢反馈时限  默认10
    uint8_t  u8SearchStepSec;    // 搜索步进周期  默认2
    uint8_t  u8SearchTimeoutSec; // 单轮搜索超时  默认30
    uint8_t  u8SearchRetryMax;   // 搜索轮数上限  默认2
    uint8_t  u8RetryStopSec;     // 轮间停转时长  默认5
} FanCfg;

// ----------------------------------------------------------------------------
// 运行实例(调用者分配;双风机:FanCtrl fan[2]各绑一个FanPort)
// ----------------------------------------------------------------------------
typedef struct
{
    // 绑定的配置与硬件端口(Init后不可更换)
    const FanCfg  *pstCfg;
    const FanPort *pstPort;

    // 状态机
    FanState eState;
    bool     bEnabled;       // 总使能
    bool     bFeedAlive;     // 本秒内是否收到有效反馈
    bool     bRetryStopping; // 搜索轮间的停转间歇标志

    // 目标与工况
    uint16_t u16TargetRpm;   // SV:目标转速(RPM)
    uint16_t u16RpmPv;       // PV:滤波后实测转速(RPM)
    uint16_t u16Output;      // 当前输出(反向刻度)

    // 反馈测速
    bool     bLastLevel;     // 上次反馈电平(边沿检测)
    uint8_t  u8PulseRemain;  // 脉冲组分频剩余计数
    uint32_t u32SpeedTicks;  // 距上次脉冲组的滴答计数
    uint32_t u32NoFeedTickLimit; // 折算成滴答数的无反馈判限

    // PID(预计算系数与历史误差)
    int32_t  s32Ka;
    int32_t  s32Kb;
    int32_t  s32Kc;
    int16_t  s16ErrPrev;
    int16_t  s16ErrLast;
    uint8_t  u8PidCount;

    // 秒级时序计数
    uint8_t  u8StartupSec;
    uint8_t  u8NoFeedSec;
    uint8_t  u8SearchSec;
    uint8_t  u8RetryStopSec;
    uint8_t  u8FailCount;
} FanCtrl;

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

/**
 * @brief  初始化实例,绑定配置与硬件钩子,预计算PID系数
 * @param  pstFan  实例指针
 * @param  pstCfg  配置(生命周期内只读)
 * @param  pstPort 硬件钩子(写占空比+读反馈电平)
 */
void FanCtrlInit(FanCtrl *pstFan, const FanCfg *pstCfg, const FanPort *pstPort);

/**
 * @brief  启动:进入启动保持期(输出锁定u16OutStartup,默认20s)
 * @note   故障锁定态拒绝启动,需先FanCtrlClearFault()
 */
void FanCtrlStart(FanCtrl *pstFan);

/** @brief 停机:输出关闭(占空比0) */
void FanCtrlStop(FanCtrl *pstFan);

/**
 * @brief  设定目标转速(SV),自动钳位到[u16RpmMin, u16RpmMax]
 * @return 成功返回true;实例指针为空返回false
 */
bool FanCtrlSetTargetRpm(FanCtrl *pstFan, uint16_t u16Rpm);

/**
 * @brief  快速节拍:挂512us级定时中断(反馈边沿检测+测速滤波+PID)
 * @note   仅Running态修改输出(单写者原则,见fanctrl.c文件头)
 */
void FanCtrlTickFast(FanCtrl *pstFan);

/** @brief 秒级节拍:挂1s周期任务(启动保持计时+丢反馈三级保护状态机) */
void FanCtrlTickSecond(FanCtrl *pstFan);

/** @brief 查询实测转速PV(RPM) */
uint16_t FanCtrlGetRpm(const FanCtrl *pstFan);

/** @brief 查询当前输出(反向刻度) */
uint16_t FanCtrlGetOutput(const FanCtrl *pstFan);

/** @brief 查询当前状态 */
FanState FanCtrlGetState(const FanCtrl *pstFan);

/** @brief 故障查询,故障锁定态返回true */
bool FanCtrlIsFault(const FanCtrl *pstFan);

/** @brief 故障解锁(回Idle),之后可重新FanCtrlStart() */
void FanCtrlClearFault(FanCtrl *pstFan);

#ifdef __cplusplus
}
#endif

#endif // FANCTRL_H

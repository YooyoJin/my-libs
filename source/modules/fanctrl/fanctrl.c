/*============================================================================
 *  fanctrl.c
 *
 *  PG/DC 风机转速闭环控制库 —— 纯逻辑实现 (C99, 零依赖)
 *
 *  设计要点:
 *    1. 输出为反向刻度: 数值越大 -> 占空比越小 -> 转速越低
 *       换算: duty(千分比) = (10000 - out*5/4) / 10
 *       其中 10000 为 2kHz 载波 PWM 的周期计数(与原工程 TMRA4 配置一致)
 *    2. 输出单写者原则(消除原工程主循环与中断互相覆盖输出的竞争):
 *       - StartupHold 态: TickSecond 负责时序, TickFast 只测速不改输出
 *       - Running    态: 仅 TickFast 中的 PID 修改输出
 *       - Search     态: 仅 TickSecond 步进修改输出
 *    3. 目标转速一律经 FanClampRpm 钳位后使用
 *==========================================================================*/
#include <stddef.h>
#include "fanctrl.h"

/* ---- 内部常量 (与原工程行为对齐, 括号内为原出处) ---- */
#define FAN_PWM_PERIOD_CNT   10000u  /* PWM 周期计数, 2kHz载波 (TimerA_PWMInit)  */
#define FAN_NOFEED_RPM       100u    /* 低于该转速视为无反馈   (PG_ERROR_SPEED)   */
#define FAN_ERR_CLAMP        400     /* PID 偏差限幅, +-       (IntTca0 中 +-400) */
#define FAN_PID_SHIFT        3       /* 增量式 PID 右移位数    (原代码 >>3)       */

/*========================================================================== * 内部函数
 *========================================================================*/
static void FanSearchTick(FanCtrl *pstFan); /* 搜索态处理, 见文件后部 */
/* 输出(反向刻度) -> 占空比千分比: (10000 - out*5/4)/10 = (40000 - 5*out)/40 */
static uint16_t FanOutToDutyPm(uint16_t u16Out)
{
    uint32_t u32Duty = ((uint32_t)FAN_PWM_PERIOD_CNT * 4u - (uint32_t)u16Out * 5u) / 40u;

    if (u32Duty > 1000u)
    {
        u32Duty = 1000u;
    }
    return (uint16_t)u32Duty;
}

/* 把当前输出写出至硬件钩子 */
static void FanWriteOutput(FanCtrl *pstFan)
{
    if (0u == pstFan->u16Output)
    {
        pstFan->pstPort->FanWriteDuty(0u); /* 0 = 关闭输出, 不走换算(对应原工程 Output<=1 特判) */
        return;
    }
    pstFan->pstPort->FanWriteDuty(FanOutToDutyPm(pstFan->u16Output));
}

/* 转速目标钳位到 [u16RpmMin, u16RpmMax] */
static uint16_t FanClampRpm(const FanCfg *pstCfg, uint16_t u16Rpm)
{
    if (u16Rpm < pstCfg->u16RpmMin)
    {
        return pstCfg->u16RpmMin;
    }
    if (u16Rpm > pstCfg->u16RpmMax)
    {
        return pstCfg->u16RpmMax;
    }
    return u16Rpm;
}

/*========================================================================== * 初始化 / 启停 / 目标设定
 *========================================================================*/
void FanCtrlInit(FanCtrl *pstFan, const FanCfg *pstCfg, const FanPort *pstPort)
{
    if ((NULL == pstFan) || (NULL == pstCfg) || (NULL == pstPort))
    {
        return;
    }

    *pstFan = (FanCtrl){0}; /* 全字段清零 */

    pstFan->pstCfg        = pstCfg;
    pstFan->pstPort       = pstPort;
    pstFan->eState        = FanStateIdle;
    pstFan->u8PulseRemain = pstCfg->u8PulseDivide;

    /* 预计算增量式 PID 系数 (对应原工程 ReInitFanKpid) */
    pstFan->s32Ka = (int32_t)pstCfg->s16PidKp + pstCfg->s16PidKi + pstCfg->s16PidKd;
    pstFan->s32Kb = -((int32_t)pstCfg->s16PidKp + 2 * (int32_t)pstCfg->s16PidKd);
    pstFan->s32Kc = (int32_t)pstCfg->s16PidKd;

    /* 无反馈滴答判限: rpm = 60e6*divide/(ticks*tickUs*perRev) < 100rpm 时的 ticks */
    pstFan->u32NoFeedTickLimit = (60000000ul * pstCfg->u8PulseDivide) /
        ((uint32_t)FAN_NOFEED_RPM * pstCfg->u16TickUs * pstCfg->u8PulsePerRev);
}

void FanCtrlStart(FanCtrl *pstFan)
{
    if ((NULL == pstFan) || (FanStateFault == pstFan->eState))
    {
        return; /* 故障锁定态需先 FanCtrlClearFault() */
    }

    pstFan->bEnabled       = true;
    pstFan->eState         = FanStateStartupHold; /* 进入启动保持期 */
    pstFan->u16Output      = pstFan->pstCfg->u16OutStartup;
    pstFan->u16TargetRpm   = FanClampRpm(pstFan->pstCfg, pstFan->u16TargetRpm);
    pstFan->u16RpmPv       = 0;
    pstFan->s16ErrPrev     = 0;
    pstFan->s16ErrLast     = 0;
    pstFan->u8PidCount     = 0;
    pstFan->u8PulseRemain  = pstFan->pstCfg->u8PulseDivide;
    pstFan->u32SpeedTicks  = 0;
    pstFan->u8StartupSec   = 0;
    pstFan->u8NoFeedSec    = 0;
    pstFan->u8SearchSec    = 0;
    pstFan->u8FailCount    = 0;
    pstFan->u8RetryStopSec = 0;
    pstFan->bRetryStopping = false;
    pstFan->bFeedAlive     = false;

    FanWriteOutput(pstFan);
}

void FanCtrlStop(FanCtrl *pstFan)
{
    if (NULL == pstFan)
    {
        return;
    }

    pstFan->bEnabled  = false;
    pstFan->eState    = FanStateIdle;
    pstFan->u16Output = 0;
    pstFan->u16RpmPv  = 0;

    FanWriteOutput(pstFan); /* duty 0 千分比 = 关闭输出 */
}

bool FanCtrlSetTargetRpm(FanCtrl *pstFan, uint16_t u16Rpm)
{
    if (NULL == pstFan)
    {
        return false;
    }

    pstFan->u16TargetRpm = FanClampRpm(pstFan->pstCfg, u16Rpm);
    return true;
}

/*========================================================================== * 快速节拍: 挂 512us 级定时中断 (测速 + PID)
 *========================================================================*/
void FanCtrlTickFast(FanCtrl *pstFan)
{
    const FanCfg  *pstCfg;
    const FanPort *pstPort;
    bool    bLevel;
    int32_t s32Err;
    int32_t s32Delta;
    int32_t s32Out;

    if ((NULL == pstFan) || (!pstFan->bEnabled))
    {
        return;
    }
    if (FanStateFault == pstFan->eState)
    {
        return;
    }

    pstCfg  = pstFan->pstCfg;
    pstPort = pstFan->pstPort;

    /* 1. 测速窗口计时 */
    pstFan->u32SpeedTicks++;

    /* 2. 反馈脉冲边沿检测 */
    bLevel = pstPort->FanReadLevel();
    if (bLevel == pstFan->bLastLevel)
    {
        return; /* 无边沿, 本拍结束 */
    }
    pstFan->bLastLevel = bLevel;

    if (--pstFan->u8PulseRemain != 0u)
    {
        return; /* 未凑满一个脉冲组 */
    }
    pstFan->u8PulseRemain = pstCfg->u8PulseDivide;

    /* 3. 一个脉冲组到 -> 计算转速并一阶滤波 (原 IntTca0: PV=(3*PV+新+2)>>2) */
    if (pstFan->u32SpeedTicks < pstFan->u32NoFeedTickLimit)
    {
        uint32_t u32Rpm = (60000000ul * pstCfg->u8PulseDivide) /
            (pstFan->u32SpeedTicks * (uint32_t)pstCfg->u16TickUs * pstCfg->u8PulsePerRev);

        pstFan->u16RpmPv   = (uint16_t)((3u * pstFan->u16RpmPv + u32Rpm + 2u) >> 2);
        pstFan->bFeedAlive = true;
    }
    pstFan->u32SpeedTicks = 0;

    /* 4. PID: 仅闭环态执行 (启动保持/搜索态只测速不动输出, 见文件头设计要点) */
    if (FanStateRunning != pstFan->eState)
    {
        return;
    }
    if (++pstFan->u8PidCount < pstCfg->u8PidDivide)
    {
        return;
    }
    pstFan->u8PidCount = 0;

    s32Err = (int32_t)pstFan->u16RpmPv - (int32_t)pstFan->u16TargetRpm;
    if (s32Err >= FAN_ERR_CLAMP)
    {
        s32Err = FAN_ERR_CLAMP;
        pstFan->s16ErrPrev = pstFan->s16ErrLast = 0; /* 限幅时清历史, 防饱和 */
    }
    else if (s32Err <= -FAN_ERR_CLAMP)
    {
        s32Err = -FAN_ERR_CLAMP;
        pstFan->s16ErrPrev = pstFan->s16ErrLast = 0;
    }
    else
    {
        /* 偏差在限内 */
    }

    /* 增量式 PID: delta = (Ka*En + Kc*En-1 + Kb*En-2) >> 3 */
    s32Delta = (pstFan->s32Ka * s32Err + pstFan->s32Kc * pstFan->s16ErrPrev +
                pstFan->s32Kb * pstFan->s16ErrLast) >> FAN_PID_SHIFT;
    pstFan->s16ErrPrev = pstFan->s16ErrLast;
    pstFan->s16ErrLast = (int16_t)s32Err;

    /* 输出限幅: 注意方向 -- 下限对应最高转速端, 上限对应最低转速端 */
    s32Out = (int32_t)pstFan->u16Output + s32Delta;
    if (s32Out < (int32_t)pstCfg->u16OutAtMaxRpm)
    {
        s32Out = pstCfg->u16OutAtMaxRpm; /* 反向刻度下限 = 最高转速 */
    }
    else if (s32Out > (int32_t)pstCfg->u16OutAtMinRpm)
    {
        s32Out = pstCfg->u16OutAtMinRpm; /* 反向刻度上限 = 最低转速 */
    }
    else
    {
        /* 在限内 */
    }

    pstFan->u16Output = (uint16_t)s32Out;
    FanWriteOutput(pstFan);
}

/*========================================================================== * 秒级节拍: 挂 1s 周期任务 (状态机时序 + 丢反馈保护)
 *========================================================================*/
void FanCtrlTickSecond(FanCtrl *pstFan)
{
    if ((NULL == pstFan) || (!pstFan->bEnabled))
    {
        return;
    }

    switch (pstFan->eState)
    {
    case FanStateStartupHold:
        if (++pstFan->u8StartupSec >= pstFan->pstCfg->u8StartupHoldSec)
        {
            pstFan->eState      = FanStateRunning; /* 保持期满, 交还 PID 闭环 */
            pstFan->u8NoFeedSec = 0;
            pstFan->bFeedAlive  = false;
        }
        break;

    case FanStateRunning:
        if (pstFan->bFeedAlive)
        {
            pstFan->bFeedAlive  = false; /* 反馈正常, 清判限计时 */
            pstFan->u8NoFeedSec = 0;
        }
        else if (++pstFan->u8NoFeedSec >= pstFan->pstCfg->u8NoFeedJudgeSec)
        {
            pstFan->eState      = FanStateSearch; /* 判丢反馈, 转搜索 */
            pstFan->u8SearchSec = 0;
        }
        else
        {
            /* 反馈尚在判限期内 */
        }
        break;

    case FanStateSearch:
        FanSearchTick(pstFan);
        break;

    case FanStateIdle:
    case FanStateFault:
    default:
        break;
    }
}

/*========================================================================== * 搜索态处理 (由 FanCtrlTickSecond 内部调用)
 *========================================================================*/
static void FanSearchTick(FanCtrl *pstFan)
{
    const FanCfg *pstCfg = pstFan->pstCfg;

    if (pstFan->bFeedAlive)
    {
        /* 搜索中找回反馈 -> 回闭环 */
        pstFan->bFeedAlive  = false;
        pstFan->eState      = FanStateRunning;
        pstFan->u8NoFeedSec = 0;
        return;
    }

    if (pstFan->bRetryStopping)
    {
        /* 两轮搜索之间的停转间歇期 */
        if (++pstFan->u8RetryStopSec >= pstCfg->u8RetryStopSec)
        {
            pstFan->bRetryStopping = false;
            pstFan->u8SearchSec    = 0;
            pstFan->u16Output      = pstCfg->u16OutStartup;
            FanWriteOutput(pstFan);
        }
        return;
    }

    pstFan->u8SearchSec++;
    if (pstFan->u8SearchSec >= pstCfg->u8SearchTimeoutSec)
    {
        /* 单轮搜索超时: 记一次失败 */
        pstFan->u8FailCount++;
        if (pstFan->u8FailCount >= pstCfg->u8SearchRetryMax)
        {
            pstFan->eState = FanStateFault; /* 轮数用尽 -> 故障锁定 */
        }
        else
        {
            pstFan->bRetryStopping = true; /* 停转间歇后重试 */
            pstFan->u8RetryStopSec = 0;
        }
        pstFan->u16Output = 0;
        FanWriteOutput(pstFan);
    }
    else if (0u == (pstFan->u8SearchSec % pstCfg->u8SearchStepSec))
    {
        /* 每 u8SearchStepSec 秒向最高转速端步进 (输出递减) */
        if (pstFan->u16Output > (uint16_t)(pstCfg->u16OutAtMaxRpm + pstCfg->u16OutSearchStep))
        {
            pstFan->u16Output -= pstCfg->u16OutSearchStep;
        }
        else
        {
            pstFan->u16Output = pstCfg->u16OutAtMaxRpm; /* 抵达全速端 */
        }
        FanWriteOutput(pstFan);
    }
    else
    {
        /* 非步进秒 */
    }
}

/*========================================================================== * 查询接口
 *========================================================================*/
uint16_t FanCtrlGetRpm(const FanCtrl *pstFan)
{
    return ((NULL == pstFan) ? 0u : pstFan->u16RpmPv);
}

uint16_t FanCtrlGetOutput(const FanCtrl *pstFan)
{
    return ((NULL == pstFan) ? 0u : pstFan->u16Output);
}

FanState FanCtrlGetState(const FanCtrl *pstFan)
{
    return ((NULL == pstFan) ? FanStateIdle : pstFan->eState);
}

bool FanCtrlIsFault(const FanCtrl *pstFan)
{
    return ((NULL == pstFan) ? false : (FanStateFault == pstFan->eState));
}

void FanCtrlClearFault(FanCtrl *pstFan)
{
    if (NULL == pstFan)
    {
        return;
    }
    if (FanStateFault == pstFan->eState)
    {
        pstFan->eState = FanStateIdle; /* 解锁, 之后可重新 FanCtrlStart() */
    }
}

/*---------------------------- End Of fanctrl.c ---------------------------*/
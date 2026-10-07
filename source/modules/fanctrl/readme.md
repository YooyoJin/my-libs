# fanctrl——PG/DC风机转速闭环控制库

版本: v0.1.0 | 标准: C99 | 依赖: 无（仅stdint/stdbool） | 源码编码: UTF-8

从Y2C022S2热泵工程 `Hardware/DC_Fan.c` 重构提炼的可移植风机控制库。
**只提炼算法与设计知识，不复制原工程代码**（原工程坑档案见notes.md）。

## 1. 文件清单

| 文件 | 说明 |
|---|---|
| fanctrl.h | 公共API：状态机 / FanPort硬件钩子 / FanCfg配置 / FanCtrl实例 |
| fancfg.h | FAN_CFG_DEFAULT默认参数预设（含原工程参数出处对照） |
| fanctrl.c | 纯逻辑实现（测速 + 增量式PID + 状态机 + 丢反馈保护） |
| readme.md | 本文档 |
| notes.md | 原工程DC_Fan.c五大坑档案 |

## 2. 控制框图

```
 目标转速(RPM) ──> [钳位200~1500] ──> SV ─┐
                                          ├─> 增量式PID ─> 输出限幅 ─> u16Output
 PG反馈脉冲 ──> [边沿检测/脉冲组分频] ─> PV ─┘        (反向刻度)      │
                [测速换算+一阶滤波]                                     ▼
                                                          FanOutToDutyPm()
                                                          duty‰=(10000-1.25*Out)/10
                                                                    │
                                                                    ▼
                                                             FanWriteDuty钩子 → PWM硬件
```

## 3. 状态机

| 状态 | 进入 | 行为 | 退出 |
|---|---|---|---|
| Idle | 上电/Stop/ClearFault | 输出关闭(0‰) | Start |
| StartupHold | Start | 输出锁定u16OutStartup（默认20s） | 保持期满→Running |
| Running | 保持期满/反馈恢复 | PID闭环，输出钳位 [u16OutAtMaxRpm, u16OutAtMinRpm] | 丢反馈10s→Search |
| Search | 丢反馈 | 每2s输出递减500搜向全速端；30s超时记失败 | 找回反馈→Running；2轮失败→Fault |
| Fault | 2轮搜索失败 | 输出关闭 | ClearFault→Idle |

**输出单写者原则**（原工程的最大隐患已修复）：
- StartupHold态：TickSecond管时序，TickFast只测速不改输出
- Running态：仅TickFast中的PID修改输出
- Search态：仅TickSecond步进修改输出

## 4. 量纲说明（重要！）

- **对外**：目标/实测转速均为RPM（u16TargetRpm / u16RpmPv）
- **对内**：u16Output为反向刻度的PWM控制量—— **数值越大占空比越小转速越低**
- 换算：`duty(千分比) = (10000 − Out×5/4) / 10`
  - Out=1500（u16OutAtMaxRpm）→ 812‰ ≈ 全速
  - Out=6700（u16OutStartup）→ 162‰ ≈ 低速启动值
  - Out=7000（u16OutAtMinRpm）→ 125‰ = 最低转速
  - Out=8000 → 0‰ = 停转
- PID限幅方向注意：**下限是最高转速端，上限是最低转速端**

## 5. API手册

| 函数 | 上下文 | 说明 |
|---|---|---|
| FanCtrlInit(pstFan, pstCfg, pstPort) | 初始化 | 绑定配置与硬件钩子，预计算PID系数 |
| FanCtrlStart(pstFan) | 任务 | 启动，进入StartupHold；Fault态拒绝 |
| FanCtrlStop(pstFan) | 任务 | 停机并关输出 |
| FanCtrlSetTargetRpm(pstFan, rpm) | 任务 | 设定目标转速，自动钳位 [200,1500] |
| FanCtrlTickFast(pstFan) | **512µs中断** | 边沿检测+测速+滤波+PID |
| FanCtrlTickSecond(pstFan) | **1s任务** | 状态机时序+丢反馈保护 |
| FanCtrlGetRpm / GetOutput / GetState | 任意 | 观测（调参/上位机） |
| FanCtrlIsFault / ClearFault | 任务 | 故障查询/解锁 |

## 6. 移植指南（HC32F460跑板）

### 6.1实现两个硬件钩子

```c
#include "fanctrl.h"
/* 对应原工程Out_FanPWM(): TMRA4 Ch3, 周期计数10000 (2kHz载波) */
static void BoardFan1WriteDuty(uint16_t u16DutyPermille)
{
    if (u16DutyPermille == 0u) { Stop_FanPWM(); }          /* 复用原函数 */
    else { TIMERA_SetCompareValue(M4_TMRA4, TimeraCh3,
           (uint16_t)((uint32_t)u16DutyPermille * 10u)); } /* ‰×10=计数 */
}
static bool BoardFan1ReadLevel(void)
{
    return PORT_GetBit(DC_FAN_PORT_FO, DC_FAN1_FO1);        /* 对应原IntTca0 */
}
static const FanPort kstFanPort1 = { BoardFan1WriteDuty, BoardFan1ReadLevel };
```

### 6.2挂接节拍

```c
/* 512µs: 在原IntTca0() (INT_TMR01_GCMB回调) 里加一行 */
void IntTca0(void) { ... FanCtrlTickFast(&kstFan1); ... }

/* 1s: 在1s任务表 (原pfTimer1sExcute) 里加一个任务函数 */
void Fan1SecondTask(void) { FanCtrlTickSecond(&kstFan1); }
```

### 6.3初始化与双风机

```c
#include "fancfg.h"
static const FanCfg kstFanCfg  = FAN_CFG_DEFAULT;
static FanCtrl kstFan1, kstFan2;            /* 双风机: 两个实例 */
FanCtrlInit(&kstFan1, &kstFanCfg, &kstFanPort1);
FanCtrlInit(&kstFan2, &kstFanCfg, &kstFanPort2);
FanCtrlSetTargetRpm(&kstFan1, 660);         /* 5挡, 查SpeedTbl[4] */
FanCtrlStart(&kstFan1);
```

### 6.4对拍建议（切换实际输出前）

先让本库与原DC_Fan.c并行"影子运行"（只调Tick、不接FanWriteDuty或输出到
空闲定时器通道），串口每秒打印GetRpm/GetOutput与原工程PV/Output(寄存器
300~305) 对比，曲线一致后再切换真实输出。

### 6.5 MDK编码注意

本库源码为UTF-8。MDK(ARMCC5) 若告警中文注释，可整体转GBK或删减注释；
ARMCLANG/ARMCC6无此问题。

## 7. 参数对照表（新 ← 旧）

| 本库FanCfg | 原工程 | 原始值 |
|---|---|---|
| u16OutAtMaxRpm | uhFanLimitL (PG_TRIG_LOWER) | 1500 |
| u16OutAtMinRpm | uhFanLimitH (PG_TRIG50_UPPER) | 7000 |
| u16OutStartup | uhFanStart (PG_START_TIME) | 6700 |
| s16PidKp/Ki/Kd | uhKp/uhKi/uhKd | 5/1/1 |
| u8PulsePerRev | PG_FEED_BACK_PLS | 12 |
| u8PulseDivide | PG_PLS_NUM_ADJ×2（按边沿计数） | 24 |
| u16RpmMin/Max | GetSpeed() 内限幅 | 200/1500 |
| u8StartupHoldSec | gucFan1OnTime>=20 | 20 (s) |
| u8NoFeedJudgeSec | ucDetectTime>=10 | 10 (s) |
| u8SearchStepSec/Step | guiPGSpeedTime 0x4000 + Out-=500 | 2s/500 |
| u8SearchTimeoutSec/Retry | PG_ERROR_HIGH_SPEED_TIME×2 | 30s×2 |
| u8RetryStopSec | PG_ERROR_STOP_TIME | 5 (s) |

## 8. 与旧实现的差异（有意为之）

1. 消除Int1_tirg(主循环) 与IntTca0(中断) 对Output的双写竞争（状态机单写者）
2. 删除"停机≥3s半速启动"(PG_HALF_TIME)，其值与全速值相同属遗留逻辑
3. 删除AC风机继电器逻辑（StartPGMotor后半段），本库专注PG/DC闭环
4. 自检快速判故障(8s)未内置，由应用层用不同FanCfg实例实现
5. 测速滴答按每拍计数（原工程每2拍），u16TickUs统一配置，公式自洽

# fanctrl —— PG/DC 风机转速闭环控制库

版本: v0.1.0 | 标准: C99 | 依赖: 无（仅 stdint/stdbool） | 源码编码: UTF-8

从 Y2C022S2 热泵工程 `Hardware/DC_Fan.c` 重构提炼的可移植风机控制库。
**只提炼算法与设计知识，不复制原工程代码**（原工程坑档案见 notes.md）。

## 1. 文件清单

| 文件 | 说明 |
|---|---|
| fanctrl.h | 公共 API：状态机 / FanPort 硬件钩子 / FanCfg 配置 / FanCtrl 实例 |
| fancfg.h | FAN_CFG_DEFAULT 默认参数预设（含原工程参数出处对照） |
| fanctrl.c | 纯逻辑实现（测速 + 增量式 PID + 状态机 + 丢反馈保护） |
| readme.md | 本文档 |
| notes.md | 原工程 DC_Fan.c 五大坑档案 |

## 2. 控制框图

```
 目标转速(RPM) ──> [钳位 200~1500] ──> SV ─┐
                                          ├─> 增量式PID ─> 输出限幅 ─> u16Output
 PG反馈脉冲 ──> [边沿检测/脉冲组分频] ─> PV ─┘        (反向刻度)      │
                [测速换算+一阶滤波]                                     ▼
                                                          FanOutToDutyPm()
                                                          duty‰=(10000-1.25*Out)/10
                                                                    │
                                                                    ▼
                                                             FanWriteDuty 钩子 → PWM硬件
```

## 3. 状态机

| 状态 | 进入 | 行为 | 退出 |
|---|---|---|---|
| Idle | 上电/Stop/ClearFault | 输出关闭(0‰) | Start |
| StartupHold | Start | 输出锁定 u16OutStartup（默认 20s） | 保持期满→Running |
| Running | 保持期满/反馈恢复 | PID 闭环，输出钳位 [u16OutAtMaxRpm, u16OutAtMinRpm] | 丢反馈 10s→Search |
| Search | 丢反馈 | 每 2s 输出递减 500 搜向全速端；30s 超时记失败 | 找回反馈→Running；2轮失败→Fault |
| Fault | 2 轮搜索失败 | 输出关闭 | ClearFault→Idle |

**输出单写者原则**（原工程的最大隐患已修复）：
- StartupHold 态：TickSecond 管时序，TickFast 只测速不改输出
- Running 态：仅 TickFast 中的 PID 修改输出
- Search 态：仅 TickSecond 步进修改输出

## 4. 量纲说明（重要！）

- **对外**：目标/实测转速均为 RPM（u16TargetRpm / u16RpmPv）
- **对内**：u16Output 为反向刻度的 PWM 控制量 —— **数值越大占空比越小转速越低**
- 换算：`duty(千分比) = (10000 − Out×5/4) / 10`
  - Out=1500（u16OutAtMaxRpm）→ 812‰ ≈ 全速
  - Out=6700（u16OutStartup）→ 162‰ ≈ 低速启动值
  - Out=7000（u16OutAtMinRpm）→ 125‰ = 最低转速
  - Out=8000 → 0‰ = 停转
- PID 限幅方向注意：**下限是最高转速端，上限是最低转速端**

## 5. API 手册

| 函数 | 上下文 | 说明 |
|---|---|---|
| FanCtrlInit(pstFan, pstCfg, pstPort) | 初始化 | 绑定配置与硬件钩子，预计算 PID 系数 |
| FanCtrlStart(pstFan) | 任务 | 启动，进入 StartupHold；Fault 态拒绝 |
| FanCtrlStop(pstFan) | 任务 | 停机并关输出 |
| FanCtrlSetTargetRpm(pstFan, rpm) | 任务 | 设定目标转速，自动钳位 [200,1500] |
| FanCtrlTickFast(pstFan) | **512µs 中断** | 边沿检测+测速+滤波+PID |
| FanCtrlTickSecond(pstFan) | **1s 任务** | 状态机时序+丢反馈保护 |
| FanCtrlGetRpm / GetOutput / GetState | 任意 | 观测（调参/上位机） |
| FanCtrlIsFault / ClearFault | 任务 | 故障查询/解锁 |

## 6. 移植指南（HC32F460 跑板）

### 6.1 实现两个硬件钩子

```c
#include "fanctrl.h"
/* 对应原工程 Out_FanPWM(): TMRA4 Ch3, 周期计数 10000 (2kHz 载波) */
static void BoardFan1WriteDuty(uint16_t u16DutyPermille)
{
    if (u16DutyPermille == 0u) { Stop_FanPWM(); }          /* 复用原函数 */
    else { TIMERA_SetCompareValue(M4_TMRA4, TimeraCh3,
           (uint16_t)((uint32_t)u16DutyPermille * 10u)); } /* ‰×10=计数 */
}
static bool BoardFan1ReadLevel(void)
{
    return PORT_GetBit(DC_FAN_PORT_FO, DC_FAN1_FO1);        /* 对应原 IntTca0 */
}
static const FanPort kstFanPort1 = { BoardFan1WriteDuty, BoardFan1ReadLevel };
```

### 6.2 挂接节拍

```c
/* 512µs: 在原 IntTca0() (INT_TMR01_GCMB 回调) 里加一行 */
void IntTca0(void) { ... FanCtrlTickFast(&kstFan1); ... }

/* 1s: 在 1s 任务表 (原 pfTimer1sExcute) 里加一个任务函数 */
void Fan1SecondTask(void) { FanCtrlTickSecond(&kstFan1); }
```

### 6.3 初始化与双风机

```c
#include "fancfg.h"
static const FanCfg kstFanCfg  = FAN_CFG_DEFAULT;
static FanCtrl kstFan1, kstFan2;            /* 双风机: 两个实例 */
FanCtrlInit(&kstFan1, &kstFanCfg, &kstFanPort1);
FanCtrlInit(&kstFan2, &kstFanCfg, &kstFanPort2);
FanCtrlSetTargetRpm(&kstFan1, 660);         /* 5挡, 查 SpeedTbl[4] */
FanCtrlStart(&kstFan1);
```

### 6.4 对拍建议（切换实际输出前）

先让本库与原 DC_Fan.c 并行"影子运行"（只调 Tick、不接 FanWriteDuty 或输出到
空闲定时器通道），串口每秒打印 GetRpm/GetOutput 与原工程 PV/Output(寄存器
300~305) 对比，曲线一致后再切换真实输出。

### 6.5 MDK 编码注意

本库源码为 UTF-8。MDK(ARMCC5) 若告警中文注释，可整体转 GBK 或删减注释；
ARMCLANG/ARMCC6 无此问题。

## 7. 参数对照表（新 ← 旧）

| 本库 FanCfg | 原工程 | 原始值 |
|---|---|---|
| u16OutAtMaxRpm | uhFanLimitL (PG_TRIG_LOWER) | 1500 |
| u16OutAtMinRpm | uhFanLimitH (PG_TRIG50_UPPER) | 7000 |
| u16OutStartup | uhFanStart (PG_START_TIME) | 6700 |
| s16PidKp/Ki/Kd | uhKp/uhKi/uhKd | 5/1/1 |
| u8PulsePerRev | PG_FEED_BACK_PLS | 12 |
| u16RpmMin/Max | GetSpeed() 内限幅 | 200/1500 |
| u8StartupHoldSec | gucFan1OnTime>=20 | 20 (s) |
| u8NoFeedJudgeSec | ucDetectTime>=10 | 10 (s) |
| u8SearchStepSec/Step | guiPGSpeedTime 0x4000 + Out-=500 | 2s/500 |
| u8SearchTimeoutSec/Retry | PG_ERROR_HIGH_SPEED_TIME×2 | 30s×2 |
| u8RetryStopSec | PG_ERROR_STOP_TIME | 5 (s) |

## 8. 与旧实现的差异（有意为之）

1. 消除 Int1_tirg(主循环) 与 IntTca0(中断) 对 Output 的双写竞争（状态机单写者）
2. 删除"停机≥3s 半速启动"(PG_HALF_TIME)，其值与全速值相同属遗留逻辑
3. 删除 AC 风机继电器逻辑（StartPGMotor 后半段），本库专注 PG/DC 闭环
4. 自检快速判故障(8s)未内置，由应用层用不同 FanCfg 实例实现
5. 测速滴答按每拍计数（原工程每 2 拍），u16TickUs 统一配置，公式自洽

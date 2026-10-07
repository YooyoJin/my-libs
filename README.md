# my-libs —— 个人嵌入式组件库

> **只收机制（mechanism），不收业务（policy）。**
> 换个机型/客户/项目会变的逻辑不放这里——那是产品工程的事。
> 判据：这段逻辑需要知道"为什么"吗？需要 → 业务，不放。

## 分层模型

```
        APP  （产品工程 / examples 的 main.c：组装根 + 业务）
          ↓ 只调 API（说抽象语言，如 RPM）
   ┌── modules/ ──────────────────────┐
   │  fanctrl  pidcore  filters  ...   │  ← 本仓库：可复用机制层
   └──────────────┬───────────────────┘
                  ↓ 只调钩子（FanPort 之类的回调）
        port/（= BSP：芯片适配层，唯一允许出现寄存器名的地方）
```

依赖铁律：**只能上层调下层，禁止横向 include，禁止成环。**
模块层内所有依赖必须在头文件 `#include` 中显式可见。

## 模块清单

| 模块 | 功能 | 版本 | 状态 |
|---|---|---|---|
| [fanctrl](modules/fanctrl/readme.md) | PG/DC 风机转速闭环（测速+增量 PID+状态机+丢反馈三级保护） | v0.1.0 | ✅ 可用（gcc/clang 零告警） |
| pidcore | 通用增量/位置式 PID | — | 📋 规划（从 fanctrl/CmpPID 中长出） |
| filters | 一阶滤波 / 滞回比较 / 斜率检测 | — | 📋 规划 |
| ringbuf | 环形缓冲（串口收发） | — | 📋 规划 |
| frameparser | 帧解析状态机（字节流→帧→回调） | — | 📋 规划 |
| scheduler | 时间片任务调度（泛化的 1s 任务表） | — | 📋 规划 |
| faultmon | 故障去抖确认 / 锁存 / 自动恢复框架 | — | 📋 规划 |
| paramservice | 参数注册表（id→范围→setter，通信与业务解耦） | — | 📋 规划 |

> 规划中的模块**不要预先实现**——第三次在业务里重复某段代码时才抽成模块。

## 目录规则

| 目录 | 放什么 | 禁止什么 |
|---|---|---|
| `modules/<name>/` | 纯 `.c/.h/.md`，模块自足 | IDE 工程文件、芯片头文件、横向 include、业务逻辑 |
| `common/` | ≥3 个模块共享的极稳定类型（准入从严） | 当杂物间 |
| `port/<芯片>/` | 寄存器操作、钩子实现 | 算法、业务 |
| `examples/` | 验证/演示工程（唯一允许 IDE 工程的地方） | 被 modules 反向依赖 |
| `docs/` | 规范与决策记录 | 与单模块绑死的文档 |
| `tools/` | 脚本工具 | — |

## 快速上手（以 fanctrl 为例）

```c
#include "fanctrl.h"
#include "fancfg.h"

static const FanCfg kstFanCfg  = FAN_CFG_DEFAULT;
static FanCtrl kstFan1;

/* 移植层只需实现两个钩子：写占空比(千分比) + 读反馈电平 */
static const FanPort kstFanPort1 = { BoardFan1WriteDuty, BoardFan1ReadLevel };

FanCtrlInit(&kstFan1, &kstFanCfg, &kstFanPort1);
FanCtrlSetTargetRpm(&kstFan1, 660);
FanCtrlStart(&kstFan1);
/* 512us 中断调 FanCtrlTickFast(); 1s 任务调 FanCtrlTickSecond(); */
```

## 版本约定

- 按模块记版本（模块 readme 头部 + CHANGELOG 段落）
- git tag 格式：`<模块>-v<版本>`，如 `fanctrl-v0.1.0`

## 规范文档

- [docs/style.md](docs/style.md) —— 命名与编码规范
- [docs/architecture.md](docs/architecture.md) —— 架构宪法（分层/判据/准入/公开策略）

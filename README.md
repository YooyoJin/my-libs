# my-libs——个人嵌入式组件库

> **只收机制（mechanism），不收业务（policy）。**
> 换个机型/客户/项目会变的逻辑不放这里——那是产品工程的事。
> 判据：这段逻辑需要知道"为什么"吗？需要 → 业务，不放。

## 分层模型

```
        APP（产品工程 / source/app：组装根 + 业务）
          ↓ 只调API（说抽象语言，如RPM）
   ┌── source/modules/ ───────────────┐
   │  fanctrl  pidcore  filters  ...   │  ← 本仓库：可复用机制层
   └──────────────┬───────────────────┘
                  ↓ 只调钩子（FanPort之类的回调）
        source/port/（= BSP：芯片适配层，唯一允许出现寄存器名的地方）
```

依赖铁律：**只能上层调下层，禁止横向include，禁止成环。**
模块层内所有依赖必须在头文件 `#include` 中显式可见。

## 模块清单

| 模块 | 功能 | 版本 | 状态 |
|---|---|---|---|
| [fanctrl](source/modules/fanctrl/readme.md) | PG/DC风机转速闭环（测速+增量PID+状态机+丢反馈三级保护） | v0.1.1 | ✅ 可用（仿真全场景验证通过） |
| [crc](source/modules/crc/readme.md) | Modbus CRC16（查表/步进/位运算，三法互证0x4B37） | v0.1.0 | ✅ 可用 |
| [log](source/modules/log/readme.md) | 分级日志（输出后端钩子化，RTT/UART可注入） | v0.1.0 | ✅ 可用 |
| pidcore | 通用增量/位置式PID | — | 📋 规划（从fanctrl/CmpPID中长出） |
| filters | 一阶滤波 / 滞回比较 / 斜率检测 | — | 📋 规划 |
| ringbuf | 环形缓冲（串口收发） | — | 📋 规划 |
| frameparser | 帧解析状态机（字节流→帧→回调） | — | 📋 规划 |
| scheduler | 时间片任务调度（泛化的1s任务表） | — | 📋 规划 |
| faultmon | 故障去抖确认 / 锁存 / 自动恢复框架 | — | 📋 规划 |
| paramservice | 参数注册表（id→范围→setter，通信与业务解耦） | — | 📋 规划 |

> 规划中的模块**不要预先实现**——第三次在业务里重复某段代码时才抽成模块。

## 目录规则

| 目录 | 放什么 | 禁止什么 |
|---|---|---|
| `source/modules/<name>/` | 纯 `.c/.h/.md`，模块自足 | IDE工程文件、芯片头文件、横向include、业务逻辑 |
| `source/common/` | ≥3个模块共享的极稳定类型（准入从严） | 当杂物间 |
| `source/port/<芯片>/` | 寄存器操作、钩子实现 | 算法、业务 |
| `source/vendor/<名>/` | 第三方SDK/RTT（**原样收，不修改一行**） | 被修改（应重新从官方获取） |
| `source/app/` | 应用层：组装根+演示业务（耦合合法区） | 业务代码出现寄存器名 |
| `projects/gcc/` | PC仿真构建(mingw, 产物fanctrldemo.exe) | 出现应用源码 |
| `projects/armgcc/` | ARM交叉**完整固件**构建（hex/bin/elf+模块.a，可直接烧录） | 出现应用源码 |
| `projects/mdk/` | MDK工程文件（唯一允许IDE工程的地方） | 出现应用源码 |
| `docs/` | 规范与决策记录 | 与单模块绑死的文档 |
| `tools/` | 脚本工具 | — |

``` cmd

E:\MyWorkspace\Project\other\blog\my-libs
├── README.md                                   ★ 总目录：机制vs业务定位/分层图/模块清单(8个)/目录规则/快速上手
├── license                                     ★ MIT
├── .gitignore                                  ★ MDK/gcc产物屏蔽
├── modules/
│   └── fanctrl/                                ★✅ 真实代码v0.1.0（5文件，gcc零告警编译通过）
│       ├── fanctrl.h / fanctrl.c / fancfg.h / readme.md / notes.md
├── common/readme.md占位：写明准入条件（≥3模块共享才进，保持空是正确状态）
├── port/
│   ├── readme.md                               BSP层规则（唯一允许寄存器名的层）
│   └── hc32f460/readme.md占位：钩子实现规划（TMRA4/反馈脚/DE脚要点都写在里面）
├── examples/
│   ├── pcdemo/readme.md占位：PC仿真验证方案（假风机模型+场景脚本设计）
│   └── hc32f460demo/readme.md占位：前后台跑板工程规划 + 对拍验证流程
├── docs/
│   ├── style.md                                ★ 命名规范：大驼峰API / u8·u16前缀 / 文件小写无下划线 / UTF-8
│   └── architecture.md                         ★ 架构宪法：三层模型 / 准入三问 / 依赖铁律 / 模块生长流程 /
│                                               绞杀者模式 / 公开策略+清洗清单
└── tools/readme.md占位：导出模块/风格自检脚本规划

```

## 快速上手（以fanctrl为例）

```c
#include "fanctrl.h"
#include "fancfg.h"

static FanCtrl kstFan1;

/* 移植层只需实现两个钩子：写占空比(千分比) + 读反馈电平 */
static const FanPort kstFanPort1 = { BoardFan1WriteDuty, BoardFan1ReadLevel };

FanCfg stCfg = FanCfgDefault();   /* 默认配置, 板级微调直接改字段 */
FanCtrlInit(&kstFan1, &stCfg, &kstFanPort1);
FanCtrlSetTargetRpm(&kstFan1, 660);
FanCtrlStart(&kstFan1);
/* 512us中断调FanCtrlTickFast(); 1s任务调FanCtrlTickSecond(); */
```

## 构建（各走各的: PC仿真/ARM交叉）

```powershell
# PC仿真(mingw, 日常用)
cmake -S projects/gcc -B projects/gcc/build -G "MinGW Makefiles"
cmake --build projects/gcc/build
.\projects\gcc\build\fanctrldemo.exe

# ARM交叉完整固件(arm-none-eabi, 产物hex/bin可直接烧录; TOOLCHAIN_FILE须绝对路径)
cmake -S projects/armgcc -B projects/armgcc/build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=<仓库绝对路径>/projects/armgcc/toolchainarmgcc.cmake
cmake --build projects/armgcc/build     # 产物: build/hc32f460demo.hex/.bin/.elf + firmware.map
# 烧录: JFlash + projects/armgcc/HC32F460PETB.jflash
```

一键: `build.bat pc` / `build.bat arm` / `build.bat all` / `build.bat run`;
VS Code: 任务`build-pc(仿真)`/`build-arm(交叉库)`(Terminal→Run Task)。

## 版本约定

- 按模块记版本（模块readme头部 + CHANGELOG段落）
- git tag格式：`<模块>-v<版本>`，如 `fanctrl-v0.1.0`

## 规范文档

- [docs/style.md](docs/style.md) ——命名与编码规范
- [docs/architecture.md](docs/architecture.md) ——架构宪法（分层/判据/准入/公开策略）

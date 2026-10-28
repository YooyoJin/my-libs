# source/——代码目录说明

本目录容纳**全部源码**，按四层组织。判断"一段代码放哪"先看两个问题：
**谁写的**（自己/第三方）+ **为谁服务**（通用机制/芯片适配/业务组装）。

```
source/
├── modules/   机制层:纯逻辑可复用模块(零芯片依赖)
├── port/      适配层:实现各模块钩子的芯片胶水代码(自写,可改)
├── vendor/    供应商层:第三方SDK/RTT(原样收,一行不改)
├── common/    共享层:≥3个模块共用的极稳定类型(准入从严,当前为空)
└── app/       应用层:组装根+业务(耦合合法区)
```

## 各层规则速查

| 层 | 放什么 | 禁止什么 | 健康判据 |
|---|---|---|---|
| `modules/<名>/` | 纯`.c/.h/.md`,机制算法 | 寄存器名、IDE工程、横向include、业务逻辑 | 拷走即编译;gcc/clang零告警 |
| `port/<芯片>/` | 钩子实现、寄存器操作 | 算法、业务、依赖app层 | 业务代码不出现的寄存器名全在这 |
| `vendor/<名>/` | 官方SDK/RTT原样拷贝 | **修改任何一行** | 想改需求→用port包一层 |
| `common/` | 多模块共享类型 | 当杂物间 | 第三次重复才准入 |
| `app/<名>/` | main组装根、业务任务、演示 | 业务代码出现寄存器名 | 业务区只说RPM/日志级别这类抽象语言 |

## 依赖方向(只准向下)

```
app  →  modules  →  (钩子) ←  port  ←  vendor
 │        │                      │
 └────────┴──→ common ←──────────┘
```

- app认识所有人;modules只认识common;port实现modules声明的钩子并使用vendor
- **禁止**:横向(modules之间不经common互include)、反向(port/app被modules include)、成环

## 当前内容一览

| 目录 | 内容 |
|---|---|
| modules/fanctrl | PG/DC风机转速闭环 v0.1.1 |
| modules/crc | Modbus CRC16(查表/步进/位运算) v0.2.0 |
| modules/log | 分级日志(输出后端钩子化) v0.1.0 |
| port/hc32f460 | 时钟/延时/LED/RTT日志后端/板级引脚表 |
| vendor/hc32f460 | 华大HC32F460 SDK(含链接脚本/启动文件) |
| vendor/seggerrtt | SEGGER RTT |
| app/pcdemo | PC端验证演示(fanctrldemo/crcdemo) |
| app/hc32f460demo | 跑板应用(OTA Bootloader骨架) |

> 构建工程不在这里——见根目录`projects/`(gcc/armgcc/mdk各走各的)。
> 完整架构规则见`docs/architecture.md`,命名规范见`docs/style.md`。

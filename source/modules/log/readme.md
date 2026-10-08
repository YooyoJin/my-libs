# log——分级日志(输出后端钩子化)

版本: v0.1.0 | 纯逻辑(仅stdio) | 移植自My-OTA工程cmn_log, 后端改为钩子注入

## 设计

模块不依赖任何硬件——输出到哪由移植层实现`LogOutputFn`后注入:

```
LOG_INFO(...) → LogPrint(格式化+过滤) → g_pfnOutput钩子 → RTT/UART/printf...
```

与fanctrl的FanPort同一思路(依赖倒置): 换输出通道不动模块。

## 接口

| 函数/宏 | 说明 |
|---|---|
| `LogInit(pfnOutput)` | 初始化+注入输出后端(传NULL丢弃全部日志) |
| `LogSetLevel(eLevel)` | 运行时过滤级别 |
| `LOG_ERROR/WARN/DEBUG/INFO(...)` | printf风格日志宏(自动带`[级别] 文件:行号`) |

配置宏(编译期): `LOG_ENABLE=0`完全移除日志代码; `LOG_SHOW_LEVEL`默认级别。

## 后端实现

- HC32F460 RTT后端: `source/port/hc32f460/rttlog.c`(RttLogInit一行拉起)

## 依赖

无(标准C库stdio/string)。

## CHANGELOG

- v0.1.0 从My-OTA移植, SEGGER_RTT直接依赖改为钩子注入

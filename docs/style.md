# style.md —— 命名与编码规范

> 范例模块：`modules/fanctrl/`。新模块开工前对照本文，提交前自查。

## 1. 文件命名

- 全小写、无下划线：`fanctrl.h`、`frameparser.c`
- 模块内文件与模块同名（配置预设可加后缀：`fancfg.h`）
- 文档统一小写：`readme.md`、`notes.md`

## 2. 函数 / API 命名

- **大驼峰**，模块名作前缀：`FanCtrlInit`、`FanCtrlTickFast`、`RingBufPush`
- 钩子/回调类型以 `Fn` 结尾：`FanWriteDutyFn`
- 模块内部 static 函数同样大驼峰：`FanSearchTick`

## 3. 变量命名：小驼峰 + 类型前缀

| 前缀 | 类型 | 示例 |
|---|---|---|
| `b` | bool | `bEnabled`、`bFeedAlive` |
| `u8` / `u16` / `u32` | 无符号整型 | `u16TargetRpm`、`u32SpeedTicks` |
| `s8` / `s16` / `s32` | 有符号整型 | `s32Ka`、`s16ErrLast` |
| `e` | 枚举 | `eState` |
| `pst` | 结构体指针 | `pstCfg`、`pstFan` |
| `kst` | const 结构体实例 | `kstFanCfg` |
| `pfn` / `pf` | 函数指针 | — |

- 结构体/枚举**类型名**大驼峰：`FanCfg`、`FanState`
- 枚举值以类型名开头：`FanStateIdle`、`FanStateRunning`
- 宏全大写下划线（仅限模块内部常量）：`FAN_PWM_PERIOD_CNT`

## 4. 编码与注释

- 源码编码 **UTF-8**
- 注释用中文，写"为什么"而不是"是什么"
- **行内/段落注释一律用 `//` 双斜杠**（不用 `/* ... */`）
- **函数注释用 Doxygen 风格**：

```c
/**
 * @brief  一句话功能
 * @param  参数名 参数说明
 * @return 返回值说明
 * @note   特殊注意点(可选)
 */
```

- 文件头用 `@file/@brief/@version`
- 分节横幅用 `// ---------- 标题 ----------`
- 结构体每个字段带注释（含单位与默认值）
- 量纲必须在注释中写明（如"反向刻度：数值越大转速越低"）

## 5. 代码风格

- C99，一处一义：`if`/`else` 均带花括号
- 指针判空写法统一：`if (NULL == pstFan)`（防止 == 误写 =）
- 常量比较左值化：`if (5u == u8X)`
- 禁止模块内出现：芯片寄存器名、`malloc`、全局变量（除 static 外）
- 编译必须通过 `-std=c99 -Wall -Wextra` 零告警（gcc 与 clang 双检）

## 6. 文档义务（每个模块必备）

| 文件 | 必含内容 |
|---|---|
| `readme.md` | 简介+接口图 / **依赖声明**（"无" 或 "pidcore>=v0.2"）/ 参数表 / CHANGELOG |
| `notes.md` | 该模块相关的踩坑记录（教学：坑 → 证据 → 教训） |

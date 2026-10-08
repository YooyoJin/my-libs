# source/port/hc32f460/——HC32F460芯片适配层(BSP)

本仓库唯一允许出现芯片寄存器名的层。移植自My-OTA工程bsp/hc32f460, 按本库风格改造。

## 文件清单

| 文件 | 说明 |
|---|---|
| bspboardtype.h | 板级引脚定义表(原样拷贝自My-OTA, 零转录风险) |
| bspclk.c/h | 时钟初始化(XTAL→MPLL 200MHz)+SysTick+回调注册 |
| bspdelay.c/h | 毫秒(SysTick节拍)/微秒(NOP忙等)延时 |
| bspled.c/h | 状态LED(心跳闪烁) |
| rttlog.c/h | log模块的RTT输出后端(SEGGER RTT通道0) |
| fanctrlport.c | 将来: fanctrl的FanPort钩子实现 |

## 改造说明

- bspdelay的毫秒延时改用LL库`SysTick_GetTick()`, 消除了原版对app层AppTimerGetTick的逆向依赖(port不得依赖app)
- 命名按style.md: 文件小写无下划线, 函数大驼峰, 注释//+Doxygen
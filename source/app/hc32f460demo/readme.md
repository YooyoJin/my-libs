# source/app/hc32f460demo/——HC32F460跑板应用(前后台)

OTA Bootloader骨架, 移植自My-OTA工程app层。应用源码放这里, 构建工程在projects/armgcc(gcc)与projects/mdk(MDK)。

## 文件

| 文件 | 说明 |
|---|---|
| main.c | 组装根: 依次拉起RTT日志→时钟→SysTick→LED→时间片调度; JumpToApp(Bootloader跳APP核心)保留备用 |
| apptimer.c/h | 时间片调度(1ms SysTick→10ms标志→1s任务表100槽分时); 第三次复用时抽成scheduler模块 |

## 前后台铁律

中断只置标志/计数, 主loop动脑子; 业务代码里不准出现寄存器名。

## 验证现象(上板)

RTT日志输出SystemCoreClock与心跳, LED周期闪烁。
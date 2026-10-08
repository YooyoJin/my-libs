# projects/armgcc/——ARM交叉完整固件构建(arm-none-eabi-gcc, Cortex-M4)

从My-OTA工程吸收构建方法, 产出**可直接烧录运行的固件**。

## 使用(在仓库根执行)

```
cmake -S projects/armgcc -B projects/armgcc/build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=<仓库绝对路径>/projects/armgcc/toolchainarmgcc.cmake
cmake --build projects/armgcc/build
```

一键: VS Code任务`build-arm(交叉库)`。

## 产物(build/目录)

| 文件 | 用途 |
|---|---|
| hc32f460demo.hex/bin | JFlash/烧录器烧录(配合同目录HC32F460PETB.jflash) |
| hc32f460demo.elf | 调试器加载(含符号) |
| firmware.map | 链接映射(占用分析) |
| libfanctrl.a/libcrc.a/liblog.a | 模块静态库 |

## 构成

- 目标: fanctrl+crc+log库 + app/hc32f460demo + port/hc32f460 + vendor(SDK/RTT) + 启动文件 + syscalls.c
- 链接脚本: vendor/hc32f460/linker/HC32F460xE.ld(512KB; 256KB芯片用-DLINKER_SCRIPT换xC)
- MCU参数: -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard(与My-OTA一致)

## 注意

- CMAKE_TOOLCHAIN_FILE必须**绝对路径**(相对路径静默失效)
- 工具链路径含空格的处理见toolchainarmgcc.cmake注释(PATH前置+裸名字)

## 上板验证(首次)

1. JFlash打开HC32F460PETB.jflash, 烧录build/hc32f460demo.hex
2. JLink RTT Viewer连接, 搜索RAM区域, 应看到`_SEGGER_RTT: 0x...`与`SystemCoreClock: 200000000Hz`日志
3. 板上LED每秒闪烁(RTT心跳日志同步输出)
# projects/armgcc/——ARM交叉构建(arm-none-eabi-gcc, Cortex-M4)

用途: 用PC上的arm交叉工具链把`source/`编译成ARM静态库(libfanctrl.a),
验证模块在目标架构下零告警编译; 将来配HC32 SDK后可产出完整固件, 作为MDK之外的命令行构建通道。

工具链: arm-gnu-toolchain-13.2(arm-none-eabi-gcc), 路径由`toolchainarmgcc.cmake`自动探测,
也可用环境变量`ARMGCC_PATH`指定bin目录。

## 使用(在仓库根执行)

```
cmake -S projects/armgcc -B projects/armgcc/build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=<仓库绝对路径>/projects/armgcc/toolchainarmgcc.cmake
cmake --build projects/armgcc/build
```

注意: `CMAKE_TOOLCHAIN_FILE`用**绝对路径**(相对路径cmake按构建目录解释,会静默失效);
一键方式: 根目录`build.bat arm`,或VS Code任务`build-arm(交叉库)`(自动用${workspaceFolder}拼绝对路径)。

产物: `projects/armgcc/build/libfanctrl.a`(ARM架构静态库)。

## 与兄弟目录的分工

| 目录 | 工具链 | 产物 | 用途 |
|---|---|---|---|
| projects/gcc | mingw(本机) | fanctrldemo.exe | PC仿真验证 |
| projects/armgcc | arm-none-eabi | libfanctrl.a | 目标架构编译验证(将来出固件) |
| projects/mdk | MDK(ac5/ac6) | 固件 | 跑板调试 |

三份工程引用同一份`../../source/`, 互不干扰, 各走各的。

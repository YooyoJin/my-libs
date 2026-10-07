# examples/gcc/pcdemo/ ——PC端仿真验证

用途：在电脑上（gcc直接编译，无需IDE和板子）验证fanctrl模块逻辑。

## 内容

- `fanctrldemo.c` ——假风机模型（转速正比于占空比 + 按12脉冲/转生成反馈电平）
  + 场景脚本：启动20s强转 → 30s换挡 → 55s拔反馈 → 搜索 → 故障锁定
  + 每秒打印曲线（t / 状态 / SV / PV / Output / duty）
- `CMakeLists.txt` ——由examples/gcc顶层统一构建

## 构建与运行（在仓库根执行）

```
cmake -S examples/gcc -B build -G "MinGW Makefiles"
cmake --build build
.\build\pcdemo\fanctrldemo.exe
```

## 验证判据（肉眼）

1. 前20s Output保持6700不动（强转期）
2. 20s后PID开始调节，PV向SV收敛
3. 30s换挡后无强转，PV追向新目标
4. 55s拔反馈 → 65s进Search → 每2s输出步进 -500
5. 两轮搜索超时后Fault锁定
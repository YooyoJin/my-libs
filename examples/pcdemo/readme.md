# examples/pcdemo/ —— PC 端仿真验证

用途：在电脑上（gcc 直接编译，无需 IDE 和板子）验证模块逻辑。

规划内容：
- `fanctrldemo.c` —— 假风机模型（转速=K×占空比 的一阶惯性环节 + 按 12 脉冲/转生成反馈电平）
  + 场景脚本（启动 20s 强转 → 换挡 → 拔反馈 → 故障 → 恢复）
  + 曲线打印（t / 状态 / SV / PV / Output / duty‰）
- `build.bat` —— `gcc -std=c99 -Wall -Wextra -o fandemo.exe ..\..\modules\fanctrl\fanctrl.c fanctrldemo.c`

验证判据（肉眼）：强转期 Output 不动、PID 收敛方向正确、丢反馈三级保护时序正确、故障可恢复。

**这不是单元测试**（无 assert/PASS-FAIL），是打印曲线的人工验证——为将来跑板前先杀掉逻辑 bug。

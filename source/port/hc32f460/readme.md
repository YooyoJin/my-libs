# port/hc32f460/

HC32F460系列（小华/HDSC）适配层。

规划中的文件：
- `fanctrlport.c` ——fanctrl的FanPort钩子实现：
  - `BoardFanWriteDuty()`: 占空比千分比 → `TIMERA_SetCompareValue(M4_TMRA4, ...)`（周期计数10000，2kHz载波，比较值 = 千分比 × 10）
  - `BoardFanReadLevel()`: `PORT_GetBit(反馈口)`
- `uart485port.c` ——USART3收发驱动 + 环形缓冲 + DE/RE方向控制
- `timertick.c` ——1ms/512µs时基中断

**规则**：这里只出现"插座"代码（每条约5~10行），控制逻辑全部在modules/ 里。

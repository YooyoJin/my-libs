# port/ —— 芯片适配层（= BSP）

**本仓库唯一允许出现芯片寄存器名的地方。**

规则：
- 只做"插座"：实现各模块声明的钩子（如 fanctrl 的 `FanPort`）
- 每种芯片一个子目录（`hc32f460/`、`stm32f103/`...），互不复用
- 禁止：算法、业务逻辑、知道 APP 在干什么
- 换芯片 = 新建一个子目录，`modules/` 一行不改

典型内容（以 hc32f460 为例）：
- `fanctrlport.c` —— 实现 FanPort 钩子（TMRA4 PWM + 反馈脚）
- `uart485port.c` —— USART3 中断 + 环形缓冲 + DE 脚方向控制（TX Complete 里切）
- `timertick.c` —— 时基中断（512µs → FanCtrlTickFast）

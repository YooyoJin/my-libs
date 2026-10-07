/*============================================================================
 *  fanctrldemo.c —— fanctrl PC 仿真验证
 *
 *  假风机模型: 转速正比于占空比, 按 12 脉冲/转生成反馈电平
 *  场景: 启动强转(20s) -> 换挡 -> 拔反馈(搜索) -> 故障锁定
 *
 *  构建: cmake -S examples/gcc -B build && cmake --build build
 *  运行: build/pcdemo/fanctrldemo.exe
 *==========================================================================*/
#include <stdio.h>
#include "fanctrl.h"
#include "fancfg.h"

static const FanCfg kstCfg = FAN_CFG_DEFAULT;
static FanCtrl  kstFan;
static uint16_t g_u16DutyPm = 0;   /* 钩子捕获的最近占空比 */
static bool     g_bLevel = false;  /* 假反馈电平 */
static bool     g_bFeedOn = true;  /* 反馈开关(模拟拔线) */

static void DemoWriteDuty(uint16_t u16DutyPm) { g_u16DutyPm = u16DutyPm; }
static bool DemoReadLevel(void)               { return g_bLevel; }

static const char *StateName(FanState eState)
{
    static const char *apcName[] = {"Idle", "StartupHold", "Running", "Search", "Fault"};
    return apcName[(int)eState];
}

int main(void)
{
    const FanPort kstPort = {DemoWriteDuty, DemoReadLevel};
    const uint32_t u32TicksPerSec = 1000000uL / 512uL; /* 512us 滴答 */
    double   dblEdgeAcc = 0.0;
    uint32_t u32Tick = 0;
    uint16_t u16Sec = 0;

    FanCtrlInit(&kstFan, &kstCfg, &kstPort);
    FanCtrlSetTargetRpm(&kstFan, 660);
    FanCtrlStart(&kstFan);

    printf("t(s)  State        SV    PV   Out   duty\n");
    for (u16Sec = 0; u16Sec <= 140; u16Sec++)
    {
        uint32_t u32End = (uint32_t)(u16Sec + 1) * u32TicksPerSec;
        for (; u32Tick < u32End; u32Tick++)
        {
            if (g_bFeedOn)
            {
                double dblRpm = (double)g_u16DutyPm * 1.2; /* 假风机: rpm ~= duty千分比 x 1.2 */
                if (dblRpm > 1500.0) { dblRpm = 1500.0; }
                dblEdgeAcc += (dblRpm * 24.0 * 512.0) / 60000000.0;
                if (dblEdgeAcc >= 1.0) { dblEdgeAcc -= 1.0; g_bLevel = !g_bLevel; }
            }
            FanCtrlTickFast(&kstFan);
        }
        FanCtrlTickSecond(&kstFan);
        printf("%4u  %-11s %4u %5u %5u  %4u\n", u16Sec, StateName(FanCtrlGetState(&kstFan)),
               kstFan.u16TargetRpm, FanCtrlGetRpm(&kstFan),
               FanCtrlGetOutput(&kstFan), g_u16DutyPm);

        if (30u == u16Sec) { FanCtrlSetTargetRpm(&kstFan, 850); } /* 换挡 */
        if (55u == u16Sec) { g_bFeedOn = false; }                  /* 拔反馈 */
    }
    return 0;
}

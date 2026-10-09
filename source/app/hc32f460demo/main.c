/**
 * @file    main.c
 * @brief   HC32F460跑板演示应用(组装根: 初始化各层+主循环)
 *
 * 本文件是"耦合合法区": 认识所有模块并完成组装接线;
 * 但业务代码里禁止出现芯片寄存器名(硬件行为经port/模块钩子)。
 *
 * 移植自My-OTA工程main.c(OTA Bootloader骨架):
 *   - RTT日志通路 + LED心跳
 *   - JumpToApp: Bootloader跳转APP的核心(保留备用)
 */
#include "main.h"
#include "hc32_ll.h"
#include "log.h"
#include "rttlog.h"
#include "bspclk.h"
#include "bspled.h"
#include "bspdelay.h"
#include "apptimer.h"

/**
 * @brief  系统初始化: 依次拉起各层(顺序有时钟依赖, 勿随意调整)
 */
static void SystemInitAll(void)
{
    LL_PERIPH_WE(LL_PERIPH_ALL);   // 解锁外写保护

    RttLogInit();                 // log模块+RTT后端
    BspClkInit();                 // 时钟(200MHz)
    BspSysTickInit();             // 1ms节拍
    BspLedInit();                 // 心跳LED
    AppTimerInit();               // 时间片调度

    LL_PERIPH_WP(LL_PERIPH_ALL);  // 重新上写保护
}

/**
 * @brief  应用入口
 * @note   main不返回(裸机主循环); 标准C要求返回int, 故返回类型为int
 */
int main(void)
{
    SystemInitAll();
    LOG_INFO("SystemCoreClock: %uHz", SystemCoreClock);
    LOG_INFO("->APP");

    // todo Bootloader防空验证(生产必备):
    // if (*(uint32_t*)APP_START_ADDR == 0xFFFFFFFF) while(1);

    while (1)
    {
        BspLedBlinking(1);   // 心跳+阻塞延时
        AppTimerTaskRun();   // 时间片任务
    }
}

/**
 * @brief  Bootloader跳转到APP
 * @param  u32AppAddr APP起始地址(该地址存放初始SP, +4存放复位向量)
 * @note   使用前应: 关全局中断+复位外设+关用到的时钟, 否则APP行为不可预期
 */
void JumpToApp(uint32_t u32AppAddr)
{
    uint32_t u32AppSp = *(volatile uint32_t *)u32AppAddr;        // APP的初始栈指针
    uint32_t u32AppPc = *(volatile uint32_t *)(u32AppAddr + 4u); // APP的复位向量
    void (*pfnAppReset)(void) = (void (*)(void))u32AppPc;

    __set_MSP(u32AppSp);   // 设置主栈指针
    pfnAppReset();         // 跳转
}


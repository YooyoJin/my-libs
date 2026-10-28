/**
 * @file    main.c
 * @brief   HC32F460跑板演示应用——前后台版(组装根)
 *
 * 本文件是耦合合法区: 认识所有模块并完成组装接线;
 * 但业务代码里禁止出现芯片寄存器名(硬件行为经port/模块钩子)。
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
    LL_PERIPH_WE(LL_PERIPH_ALL);  // 解锁外写保护

    RttLogInit();                 // log模块+RTT后端
    BspClkInit();                 // 时钟(200MHz)
    BspLedInit();                 // LED
    AppTimerInit();               // 时间片调度(内部初始化SysTick)

    LL_PERIPH_WP(LL_PERIPH_ALL);  // 重新上写保护
}

/**
 * @brief  应用入口(前后台: 主循环轮询)
 */
int main(void)
{
    SystemInitAll();
    LOG_INFO("SystemCoreClock: %uHz", SystemCoreClock);
    LOG_INFO("->APP");

    while (1)
    {
        BspLedBlinking(1);   // 心跳+阻塞延时
        AppTimerTaskRun();   // 时间片任务
    }
}

/**
 * @brief  Bootloader跳转到APP
 * @param  u32AppAddr APP起始地址(该地址存放初始SP, +4存放复位向量)
 * @note   使用前应: 关全局中断+复位外设+关用到的时钟
 */
void JumpToApp(uint32_t u32AppAddr)
{
    uint32_t u32AppSp = *(volatile uint32_t *)u32AppAddr;
    uint32_t u32AppPc = *(volatile uint32_t *)(u32AppAddr + 4u);
    void (*pfnAppReset)(void) = (void (*)(void))u32AppPc;

    __set_MSP(u32AppSp);
    pfnAppReset();
}
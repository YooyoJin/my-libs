/**
 * @file    bspled.c
 * @brief   状态LED控制实现(BSP层)
 * @note    移植自My-OTA工程bsp_led.c, 逻辑未改动
 */
#include "bspled.h"
#include "bspdelay.h"
#include "bspboardtype.h"
#include "hc32_ll_gpio.h"
#include "log.h"

static BspGpioPin_T g_stStatusLed;

/** @brief LED初始化: 输出模式+默认熄灭 */
void BspLedInit(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir  = PIN_DIR_OUT;
    stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL;
    g_stStatusLed.u16Port  = BSP_LED0_PORT;
    g_stStatusLed.u16Pin   = BSP_LED0_PIN;
    (void)GPIO_Init(g_stStatusLed.u16Port, g_stStatusLed.u16Pin, &stcGpioInit);

    BspLedOff();
}

/** @brief 点亮(低电平驱动) */
void BspLedOn(void)
{
    GPIO_ResetPins(g_stStatusLed.u16Port, g_stStatusLed.u16Pin);
}

/** @brief 熄灭 */
void BspLedOff(void)
{
    GPIO_SetPins(g_stStatusLed.u16Port, g_stStatusLed.u16Pin);
}

/** @brief 翻转 */
void BspLedToggle(void)
{
    GPIO_TogglePins(g_stStatusLed.u16Port, g_stStatusLed.u16Pin);
}

/** @brief 阻塞闪烁: 每次亮灭各500ms, 期间输出心跳日志 */
void BspLedBlinking(uint8_t u8Times)
{
    static uint32_t u32Count = 0;

    BspLedOff();
    while (u8Times--)
    {
        LOG_DEBUG("Heart beats, %u", u32Count++);
        BspLedToggle();
        BspDelayMs(500);
        BspLedToggle();
        BspDelayMs(500);
    }
}

/**
 * @file    bspclk.h
 * @brief   HC32F460时钟初始化+SysTick(BSP层)
 */
#ifndef BSPCLK_H
#define BSPCLK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @brief SysTick回调函数类型(1ms触发一次) */
typedef void (*SysTickCallbackFn)(void);

/** @brief 系统时钟初始化: XTAL→MPLL 200MHz主频+总线分频+SRAM/Flash等待周期 */
void BspClkInit(void);

/** @brief SysTick初始化(1ms节拍) */
void BspSysTickInit(void);

/** @brief 注册SysTick回调(在1ms中断上下文执行, 只准置标志/计数) */
void BspSysTickRegisterCallback(SysTickCallbackFn pfnCallback);

#ifdef __cplusplus
}
#endif

#endif // BSPCLK_H

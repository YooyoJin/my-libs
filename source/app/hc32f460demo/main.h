/**
 * @file    main.h
 * @brief   HC32F460跑板演示应用(组装根)
 */
#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 标准C要求main返回int(armclang严格执行; gcc嵌入式传统void main不合规)
int main(void);

/**
 * @brief  Bootloader跳转到APP(见main.c)
 * @param  u32AppAddr APP起始地址(该处存初始SP, +4存复位向量)
 */
void JumpToApp(uint32_t u32AppAddr);

#ifdef __cplusplus
}
#endif

#endif // MAIN_H

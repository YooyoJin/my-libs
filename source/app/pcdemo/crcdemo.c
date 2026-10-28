/**
 * @file    crcdemo.c
 * @brief   crc模块PC演示: 标准校验值/Modbus填帧字节序/接收端校验技巧
 *
 * 构建: cmake --build projects/gcc/build
 * 运行: projects/gcc/build/crcdemo.exe
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "crc.h"

/** @brief 打印字节数组 */
static void PrintBytes(const char *pcTag, const uint8_t *pu8Data, uint16_t u16Len)
{
    uint16_t u16i;

    printf("%s", pcTag);
    for (u16i = 0; u16i < u16Len; u16i++) {
        printf("%02X ", pu8Data[u16i]);
    }
    printf("\n");
}

int main(void)
{
    uint8_t  au8Frame[8];
    uint16_t u16Crc;
    uint16_t u16i;
    uint8_t  u8Ok = 1u;

    printf("=== 1. 标准校验值(Modbus CRC16) ===\n");
    {
        const uint8_t au8Std[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        uint16_t      u16A = Crc16Modbus(au8Std, 9);
        uint16_t      u16B = Crc16ModbusBitwise(au8Std, 9);
        uint16_t      u16C = 0xFFFF;  // 步进状态, 初值0xFFFF

        for (u16i = 0; u16i < 9; u16i++) {
            Crc16ModbusStep(au8Std[u16i], &u16C);
        }

        printf("查表=0x%04X 位运算=0x%04X 步进=0x%04X 期望=0x4B37  %s\n",
               u16A,
               u16B,
               u16C,
               (u16A == 0x4B37u && u16A == u16B && u16A == u16C) ? "PASS" : "FAIL");

        if (!((u16A == 0x4B37u) && (u16A == u16B) && (u16A == u16C))) {
            u8Ok = 0u;
        }
    }

    printf("\n=== 2. 真实Modbus帧: 字节序是生死线 ===\n");
    {
        // 主机请求: 从站01 读保持寄存器(功能码03) 起始0000 数量0001
        uint8_t au8Req[6] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};

        u16Crc = Crc16Modbus(au8Req, 6);
        printf("请求帧: 01 03 00 00 00 01, CRC寄存器值=0x%04X\n", u16Crc);
        printf("Modbus规范: 上送时[CRC低][CRC高]——低字节在前!\n\n");

        // 正确: 小端填入
        memcpy(au8Frame, au8Req, 6);
        au8Frame[6] = (uint8_t)(u16Crc & 0xFF);
        au8Frame[7] = (uint8_t)(u16Crc >> 8);
        PrintBytes("正确(小端): ", au8Frame, 8);

        // 错误: 大端填入
        au8Frame[6] = (uint8_t)(u16Crc >> 8);
        au8Frame[7] = (uint8_t)(u16Crc & 0xFF);
        PrintBytes("错误(大端): ", au8Frame, 8);
    }

    printf("\n=== 3. 接收端校验技巧: CRC并入后再算, 恒为0 ===\n");
    {
        // CRC本质时模2除法余数
        // 算CRC就是喂原始数据，计算得余数，把余数附在数据后面发出去（modbus crc 必须调整顺序，低位在前）。
        // 收CRC就是时把原始数据+CRC，再计算余数，这样如果数据正确得话自然没有余数。
        uint16_t u16Check;

        // 正确帧: 含小端CRC的整帧再算CRC应恒等于0
        memcpy(au8Frame, (uint8_t[]){0x01, 0x03, 0x00, 0x00, 0x00, 0x01}, 6);
        au8Frame[6] = (uint8_t)(u16Crc & 0xFF);  // CRC低字节在前(小端)
        au8Frame[7] = (uint8_t)(u16Crc >> 8);    // CRC高字节在后
        u16Check = Crc16Modbus(au8Frame, 8);
        printf("正确帧整帧CRC = 0x%04X (期望0x0000)  %s\n", u16Check, (0u == u16Check) ? "PASS" : "FAIL");
        if (0u != u16Check) {
            u8Ok = 0u;
        }

        // 错误帧(大端填入): 整帧CRC必不为0 → 从机判校验失败
        au8Frame[6] = (uint8_t)(u16Crc >> 8);    // CRC高字节在前(大端)
        au8Frame[7] = (uint8_t)(u16Crc & 0xFF);  // CRC低字节在前
        u16Check = Crc16Modbus(au8Frame, 8);
        printf("错误帧整帧CRC = 0x%04X (必非0)      %s\n", u16Check, (0u != u16Check) ? "PASS" : "FAIL");
        if (0u == u16Check) {
            u8Ok = 0u;
        }
    }

    printf("\n结论: %s\n", u8Ok ? "全部PASS" : "存在FAIL");
    return (u8Ok ? 0 : 1);
}

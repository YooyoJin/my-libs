# crc——Modbus CRC16校验

版本: v0.1.0 | 纯逻辑零依赖 | 移植自My-OTA工程cmn_crc并按本库风格改造

## 接口

| 函数 | 说明 |
|---|---|
| `Crc16Modbus(pu8Msg, u16Len)` | 查表法整块计算(推荐) |
| `Crc16ModbusStep(u8Data, &u8Hi, &u8Lo)` | 查表法单字节步进(流式) |
| `Crc16ModbusBitwise(pu8Msg, u16Len)` | 位运算法(不依赖查找表, 省ROM) |

三者结果一致; 标准校验值: `Crc16Modbus("123456789",9)==0x4B37`。

## 依赖

无。

## 与原实现的差异(重要)

原My-OTA工程查表法按`(高<<8|低)`拼装, 返回值与位运算法**字节序对调**(遗留隐患);
本模块已修正拼装顺序, 三算法互证一致。Modbus发送时低字节在前。

## CHANGELOG

- v0.1.0 从My-OTA移植, 修正查表法字节序问题, 命名按style.md改造

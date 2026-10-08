# source/app/pcdemo/——PC端验证演示集

用途: 在电脑上(gcc直接编译, 无需IDE和板子)验证各模块。

## 演示清单

| 可执行 | 验证对象 | 判据 |
|---|---|---|
| fanctrldemo.exe | fanctrl | 强转20s→PID收敛→换挡→拔反馈→搜索→Fault, 全状态流转 |
| crcdemo.exe | crc | 标准校验值0x4B37三法一致; Modbus小端填帧整帧复算=0x0000; 大端错误填法必非0 |

## 构建与运行(在仓库根执行, 工程文件位于projects/gcc)

```
cmake -S projects/gcc -B projects/gcc/build -G "MinGW Makefiles"
cmake --build projects/gcc/build
.\projects\gcc\build\fanctrldemo.exe
.\projects\gcc\build\crcdemo.exe
```

## crcdemo重点: Modbus字节序是生死线

- CRC寄存器值按[低字节][高字节]上送(小端线序)
- 接收端校验技巧: 把CRC并入整帧再算, 结果恒为0x0000即为合法帧
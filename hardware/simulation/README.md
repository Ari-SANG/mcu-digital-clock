# Proteus 参考仿真

[项目首页](../../README.md) · [硬件差异](../../docs/hardware.md)

- [Proteus 原理参考 PDF](【仿真】Proteus_DIGITAL_CLOCK_V4.2.pdf)：一页仿真线路图。

原可编辑工程仅保存在本地，未随本次附件公开。该原工程 MCU 的 Program File 记录为 `C:\Users\18804\Desktop\clock\Objects\clock1.hex`。它不是仓库内文件。打开工程后双击 MCU，将 Program File 指向你重新构建的 `firmware/Objects/Uart1_Demo.hex`，核对实际时钟参数为 11.0592 MHz。原工程不作已验证的当前固件仿真声明。

参考仿真图使用 **LM35**，当前实物固件使用 **NTC 分压 + 线性近似换算**。不能把旧仿真温度直接作为当前 NTC 算法的验收结果；需要替换温度模型及对应连线后再验证。数码管、按键和 UART 的引脚也应按 `docs/hardware.md` 逐项核对。

使用 STC15 仿真模型时需相应 Proteus 组件库；仓库没有可确认的独立模型库文件，公开资料只有参考 PDF，本机另存原工程。缺失组件时从课程/原模型来源恢复，不把泛用 8051 的定时器行为当作 STC 1T 自动重装载和时钟输出的等价实现。

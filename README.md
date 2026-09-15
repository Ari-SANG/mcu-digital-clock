# 多功能数字钟 · mcu-digital-clock

基于 STC15W4K 系列 8051 单片机与 ZJNU EELab V1.3 数字钟板的课程设计工程。项目原有说明标注实物为 **STC15W4K48S4**；课程参考电路和 Keil 器件选择为 **STC15W4K32S4**。系统时钟按 **11.0592 MHz** 配置，串口为 **115200 / 8N1**。替换硬件时先核对芯片丝印、引脚和下载时钟。

本仓库同时保存当前固件、电脑串口助手、硬件电路、原理资料和历史版本，方便以后从源码重新编译、理解接线并恢复使用。

![PCB V1.3 预览](hardware/images/【PCB效果图】多功能数字钟--3D--V1.3.png)

## 一年后重新打开，从这里开始

1. [阅读导航与目录说明](docs/README.md)：先分清当前工程、参考资料和历史归档。
2. [硬件原理与接线](docs/hardware.md)：确认供电、数码管、按键、蜂鸣器和串口。
3. [程序架构与原理](docs/architecture.md)：沿着 `main → app → Drivers` 阅读。
4. [编译、烧录与恢复](docs/build-and-flash.md)：从干净克隆重新生成 HEX 和上位机。
5. [按键与功能操作](docs/usage.md)、[串口协议](docs/protocol.md)：实际使用与二次开发。
6. [背景资料索引](docs/references/README.md)：按问题查找电路资料与数据手册章节。

## 功能

- 四位数码管显示时间，HH:MM 与 MM:SS 每 3 秒交替；时钟与温度空闲每 10 秒切换。
- 学号滚动与末四位固定显示；按键设置时、分、秒，支持长按连发。
- 三组独立闹钟、三首可选旋律、试听和 10 分钟贪睡；可开关每秒滴答音。
- 串口校时、闹钟设置、温度查询、自动息屏开关与立即息屏。
- EEPROM 保存确认后的时间快照、闹钟与自动息屏设置；掉电期间不继续计时。
- C# WinForms 串口助手；另有较早的 Python 校时工具。

## 目录

```text
firmware/                  当前可编译的 Keil C51 工程
  main.c                   初始化和主循环
  config.h                 时钟频率、学号、默认值和时序参数
  Application/             显示状态、编辑、串口事件与息屏
  Drivers/                 显示计时、按键、音乐、UART、EEPROM
  vendor/STC15.H           工程所需 STC 寄存器头文件
  Uart1_Demo.uvproj         Keil 工程入口（保留历史工程名）
tools/
  serial-assistant/        完整 C# 上位机源码、图标与构建脚本
  python/                  Python/Tkinter 基础校时助手
hardware/
  images/                  PCB 3D 图
  simulation/              Proteus 参考 PDF
docs/
  references/              STC15 中文数据手册与资料索引
archive/                   原始演示、早期未完成固件与构建历史
artifacts/                 重建日志与验证记录
```

## 最快恢复

```powershell
git clone https://github.com/Ari-SANG/mcu-digital-clock.git
cd mcu-digital-clock
```

安装带 **C51** 的 Keil uVision，打开 [firmware/Uart1_Demo.uvproj](firmware/Uart1_Demo.uvproj)，Rebuild 后烧录 `firmware/Objects/Uart1_Demo.hex`，下载工具的系统时钟设为 **11.0592 MHz**。上位机进入 `tools/serial-assistant` 后运行 `build.bat`。详细步骤与故障排查见[恢复说明](docs/build-and-flash.md)。

当前主工程只有 `firmware/`。`archive/early-firmware/` 是未完成的早期尝试，不能直接作为当前工程编译。原始参考仿真采用不同温度器件并含旧电脑路径，使用前按[仿真说明](hardware/simulation/README.md)调整。

## 归档记录

2026-09-15：重新组织目录，纳入原来位于仓库外的参考资料；保留已有未提交的立即息屏功能与串口助手改动，补齐本地依赖头文件。构建输出与个人 IDE 状态不再作为日常源码跟踪；构建验证日志保存在 `artifacts/`。

资料来源、版本差异和归档边界见[资料索引](docs/references/README.md)与[归档说明](archive/README.md)。厂商头文件及旋律素材保留各自来源，本次整理不为第三方材料新增授权。

本次随工程公开的原始背景附件仅包含你确认的 PCB V1.3 预览、电路 PDF、Proteus 仿真 PDF 和 STC15 数据手册。完整课程 PDF、报告模板、系统框图、可编辑仿真副本及新构建的 HEX/EXE 留在本地；从仓库源码可重新生成当前固件和上位机。

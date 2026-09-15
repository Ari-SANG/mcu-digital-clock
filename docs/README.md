# 工程阅读导航

[回到项目首页](../README.md)

## 建议阅读路线

| 想解决的问题 | 先读文档 | 再看文件 |
| --- | --- | --- |
| 这是什么板、怎样接线？ | [硬件说明](hardware.md) | `hardware/` 电路图、PCB 图 |
| 怎样重新编译和烧录？ | [构建与恢复](build-and-flash.md) | `firmware/Uart1_Demo.uvproj`、`config.h` |
| 主循环与中断怎样配合？ | [架构原理](architecture.md) | `main.c` → `Application/app.c` → `Drivers/board.c` |
| 怎样切换显示、修改时间？ | [功能操作](usage.md) | `app.c`、`keys.c` |
| 电脑怎样和单片机通信？ | [协议表](protocol.md) | `uart1.c`、`tools/serial-assistant/MainForm.cs` |
| 断电保存如何实现？ | [架构原理](architecture.md) | `Drivers/settings.c` |
| 电路与芯片寄存器从哪里查？ | [背景索引](references/README.md) | 电路 PDF、STC15 数据手册 |
| 哪些属于旧版本？ | [归档说明](../archive/README.md) | `archive/`，以及 Git 历史 |

文档路径中的固件文件均相对于 `firmware/`。`Uart1_Demo` 是历史遗留输出名，实际工程已实现完整数字钟功能。

## 目录迁移对照（2026-09-15）

| 原位置 | 新位置 |
| --- | --- |
| 根目录 `main.c`、`config.h`、`Application/`、`Drivers/`、Keil 工程 | `firmware/`，内部相对路径保留 |
| `chuankou/`，本地后改名为 `串口助手/` | `tools/serial-assistant/` |
| `PC_Tools/` | `tools/python/` |
| 根目录 `*.log` | `archive/build-logs/` |
| 原始 `.bak` 文件 | `archive/original-demo/` |
| 仓库外的电路图、PCB 图、仿真工程 | `hardware/` |
| 仓库外的数据手册 | `docs/references/` |
| 仓库外的早期 `firmware/src`、`firmware/include` | `archive/early-firmware/` |
| `Objects/`、`Listings/` | 本地 `firmware/` 内保留，Git 忽略；验证日志放 `artifacts/` |

源码的原有注释编码保留；新文档与 `vendor/STC15.H` 使用 UTF-8。历史 `.bak` 中有 GBK 文件，阅读时使用相应编码，避免批量转换造成差异。

原有详尽英文功能说明保存在 [feature-guide.md](feature-guide.md)，中文操作与协议文档为新目录的主要入口。

本机最外层也已整理：原始材料保留在 `本地原始资料/`，ISP 程序与快捷方式保留在 `本地工具/`；实际 Git 仓库仍为 `mcu digital clock/`。这些本地备份目录不影响 GitHub 克隆恢复。

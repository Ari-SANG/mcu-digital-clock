# 历史归档

[项目首页](../README.md)

## original-demo

原始串口演示备份 `main_uart_demo_original_gbk.c.bak`、ACK 修改前的 UART 备份 `uart1_ansi_before_ack.c.bak`。来源文件原样保留；部分源码注释使用 GBK。课程参考测试 HEX 仅留在本地，不属于本次公开附件。

## early-firmware

原工作目录另有一份 `firmware/src`、`firmware/include`，属于早期分层实现：Timer0 扫描/走时，与当前 Timer1 扫描 + Timer0 音乐架构不同。仅归档源码与头文件。

**该版本不完整**：`main.c` 调用 `ui_init()`、`ui_handle_keys()` 等函数，但原目录没有 `ui.c`，也没有完整 Keil 工程入口。原编译目录中的 `ui.obj` / `ui_test.obj` 不能替代缺失源码。不要将它与 `firmware/` 主工程混合编译。

## build-logs

保留已有阶段性编译日志，作为功能演进线索；其中可能记录原开发机路径。它们不是本次全新验证结果。本次归档构建记录保存在 `artifacts/`。

## Git 历史

原提交历史没有重写；旧目录中的编译输出和 EXE 仍可在旧提交查到。本次仅从当前树移除日常生成文件，构建结果在本机保留，GitHub 仅保存验证记录。回看旧提交时按当时的目录结构打开工程。

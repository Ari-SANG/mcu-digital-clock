# 构建验证记录 · 2026-09-15

[项目首页](../README.md) · [重新构建](../docs/build-and-flash.md)

- [keil-build.txt](firmware/keil-build.txt)：迁移后的全量构建日志，0 错误、0 警告，data=127.5、xdata=20、code=4775 字节。
- [restore-build.txt](firmware/restore-build.txt)：无编译缓存的已提交文件副本全量重建，0 错误、0 警告；生成 HEX 与本地归档快照逐字节一致。
- [SHA256SUMS.txt](SHA256SUMS.txt)：上述两份日志的 SHA-256。

C# WinForms 串口助手也从无缓存副本重新编译通过。当前 HEX/EXE 留在本机，公开仓库保存完整源码和重建说明。

本次未进行实物烧录、串口交互、24 小时走时、温度标定或 Proteus 动态仿真。

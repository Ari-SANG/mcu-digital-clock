# 编译、烧录与恢复

[阅读导航](README.md) · [硬件接线](hardware.md)

## 固件环境

- Windows 与 Keil uVision，安装 **C51 编译工具链**。仅安装 ARM 的 MDK 不够。
- 当前项目文件为 `firmware/Uart1_Demo.uvproj`，使用 MCS-51 工具集；Device 沿用 STC15W4K32S4 Series，实物型号按丝印确认。
- STC 器件数据库仍需在 Keil 注册；若提示找不到 STC 器件，从 STC 官方工具包安装对应器件数据库。
- 项目依赖的 `STC15.H` 已随仓库放在 `firmware/vendor/`，IncludePath 已添加该目录，无需沿用原电脑头文件路径。
- 编译器标准头文件、C51 库和启动代码由 Keil 提供。`vendor/README.md` 记录厂商头文件来源。

## 用 Keil 重新构建

1. 打开 `firmware/Uart1_Demo.uvproj`。
2. 核对 `firmware/config.h`：`FOSC=11059200L`、`UART1_BAUD=115200L`、学号与默认闹钟。
3. Rebuild All Target Files；确认没有错误，处理任何警告。
4. 输出 HEX：`firmware/Objects/Uart1_Demo.hex`；清单与链接映射：`firmware/Listings/`。

工程整体移动到 `firmware/`，其内部相对源文件路径没有变化。Keil 的调试 CPU 时钟从历史 35 MHz 对齐为 11.0592 MHz；实际 MCU 时钟由硬件与下载设置决定。

可从 PowerShell 用自己安装的路径重建，例如：

```powershell
$keilExe = 'D:\MDK5.36\UV4\UV4.exe'
$projectFile = (Resolve-Path '.\firmware\Uart1_Demo.uvproj').Path
$buildLog = Join-Path (Get-Location) 'firmware\build.log'
Start-Process -FilePath $keilExe -ArgumentList ('-r "{0}" -o "{1}"' -f $projectFile, $buildLog) -WindowStyle Hidden -Wait
Get-Content $buildLog
```

`Objects/`、`Listings/`、个人 `.uvopt`/`.uvgui.*` 和新日志由 `.gitignore` 忽略，重新构建会生成它们。历史日志单独保留在 `archive/build-logs/`。

## 烧录

1. 接好供电、UART 与共地，核对 H2 显示电源跳线。
2. 在 STC-ISP / AiCube-ISP 中选择实际芯片型号、正确 COM 口与 HEX。
3. 设置系统时钟 **11.0592 MHz**，保持与 `FOSC` 一致。
4. 需要保留设置时配置为**不擦除 EEPROM**；配置低压禁止 EEPROM 操作选项。要验证首次启动默认值时，可明确擦除 EEPROM 后重新开始。
5. 按下载工具提示进行下载及必要的重新上电，确认下载成功。
6. 关闭下载工具对串口的占用，再启动上位机。

仓库不携带 Keil 或 ISP 安装器：原工作目录中的安装工具、快捷方式和下载日志不是恢复工程所需源码。从厂商获取工具，参考随仓库保留的数据手册第 16 章。

## C# 串口助手

主版本位于 `tools/serial-assistant/`，使用 WinForms 和 .NET Framework；项目目标为 **.NET Framework 4.8**。

双击 `build.bat` 会调用 Windows .NET Framework 的 `csc.exe`，生成 `bin/SerialTimeSync.exe`；脚本以自己的目录为工作目录，所以项目搬移不影响构建。也可用 MSBuild 打开 `SerialTimeSync.csproj`，需安装对应 4.8 开发组件。图标构建所需 ICO 已随源码保存。

重新生成图标才需要 Python + Pillow；普通 C# 构建不需要 Python。EXE 在本地重建，GitHub 保存源码与构建说明。

## Python 基础校时助手

```powershell
py -m pip install -r tools/python/requirements.txt
py tools/python/serial_time_sync.py
```

需要带 Tkinter 的 Python。这个较早的工具只覆盖基础校时，不包含 C# 助手的全部闹钟、温度与息屏页面；其成功提示不能代替设备 ACK 验证。

## 仿真和验证记录

[Proteus 参考说明](../hardware/simulation/README.md)记录旧资料与当前固件的差异。`artifacts/` 保存构建日志与干净恢复验证记录，见 [artifacts/README.md](../artifacts/README.md)。当前 HEX 与 EXE 由源码在本地生成。

## 常见问题

| 现象 | 先检查 |
| --- | --- |
| 找不到 STC15.H | `vendor/STC15.H` 是否存在，IncludePath 是否有 `.\vendor` |
| 不认识 C51 语法或工具集 | 是否装了 Keil C51，不能用普通桌面 C 编译器替代 |
| 上位机无 ACK | COM 占用、TX/RX、共地、波特率、时钟配置、是否烧录当前 HEX |
| 校时可用但温度查询失败 | 是否还在使用不支持命令 05 的旧 HEX |
| 断电后时间落后 | 只恢复最近确认保存的快照，没有电池 RTC |
| 修改默认值后未生效 | EEPROM 中已有有效记录，默认值只在无有效记录时使用 |
| 仿真找不到 HEX | 编辑 MCU 的 Program File，旧工程含外部绝对路径 |

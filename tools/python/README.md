# Python 基础校时工具

`serial_time_sync.py` 是较早的 Python/Tkinter 校时助手，支持电脑时间与手动时间发送，使用 `01 HH MM SS AA` BCD 帧和 115200、8N1。

```powershell
py -m pip install -r requirements.txt
py serial_time_sync.py
```

需要带 Tkinter 的 Python；桌面串口需要设备驱动。该版本只作基础校时入口，完整的闹钟、温度、息屏及 ACK 确认功能请使用 [C# 串口助手](../serial-assistant/README.md)。

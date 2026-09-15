"""STC15 digital-clock serial time synchronization tool.

Protocol: 01 HH MM SS AA, where HH/MM/SS are packed BCD bytes.
Serial settings: 115200 baud, 8 data bits, no parity, 1 stop bit.
"""

from __future__ import annotations

import tkinter as tk
from datetime import datetime
from tkinter import messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


BAUD_RATE = 115200
FRAME_HEAD = 0x01
FRAME_TAIL = 0xAA


def to_bcd(value: int) -> int:
    """Convert a decimal value from 0 to 99 to packed BCD."""
    if not 0 <= value <= 99:
        raise ValueError("BCD 数值必须在 0 到 99 之间")
    return ((value // 10) << 4) | (value % 10)


def build_time_frame(hour: int, minute: int, second: int) -> bytes:
    """Build and validate one time synchronization frame."""
    if not 0 <= hour <= 23:
        raise ValueError("小时必须在 0 到 23 之间")
    if not 0 <= minute <= 59:
        raise ValueError("分钟必须在 0 到 59 之间")
    if not 0 <= second <= 59:
        raise ValueError("秒必须在 0 到 59 之间")
    return bytes(
        (FRAME_HEAD, to_bcd(hour), to_bcd(minute), to_bcd(second), FRAME_TAIL)
    )


class SerialTimeSyncApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("单片机串口时间同步")
        self.root.geometry("560x430")
        self.root.minsize(520, 400)

        self.port_var = tk.StringVar()
        self.pc_time_var = tk.StringVar()
        self.status_var = tk.StringVar(value="请选择串口")
        self.frame_var = tk.StringVar(value="尚未发送数据")
        now = datetime.now()
        self.hour_var = tk.StringVar(value=f"{now.hour:02d}")
        self.minute_var = tk.StringVar(value=f"{now.minute:02d}")
        self.second_var = tk.StringVar(value=f"{now.second:02d}")

        self._configure_style()
        self._build_ui()
        self.refresh_ports()
        self._update_pc_clock()

        if serial is None:
            self.status_var.set("缺少 pyserial，请先安装依赖")
            self.root.after(150, self._show_dependency_error)

    def _configure_style(self) -> None:
        style = ttk.Style()
        if "vista" in style.theme_names():
            style.theme_use("vista")
        style.configure("Title.TLabel", font=("Microsoft YaHei UI", 18, "bold"))
        style.configure("Clock.TLabel", font=("Consolas", 22, "bold"))
        style.configure("Status.TLabel", font=("Microsoft YaHei UI", 10))
        style.configure("Accent.TButton", font=("Microsoft YaHei UI", 11, "bold"))

    def _build_ui(self) -> None:
        outer = ttk.Frame(self.root, padding=20)
        outer.pack(fill="both", expand=True)

        ttk.Label(outer, text="串口时间同步工具", style="Title.TLabel").pack(
            anchor="w", pady=(0, 16)
        )

        port_box = ttk.LabelFrame(outer, text="串口设置", padding=12)
        port_box.pack(fill="x")
        port_box.columnconfigure(1, weight=1)
        ttk.Label(port_box, text="串口：").grid(row=0, column=0, sticky="w")
        self.port_combo = ttk.Combobox(
            port_box, textvariable=self.port_var, state="readonly", width=30
        )
        self.port_combo.grid(row=0, column=1, sticky="ew", padx=(8, 8))
        ttk.Button(port_box, text="刷新串口", command=self.refresh_ports).grid(
            row=0, column=2
        )
        ttk.Label(port_box, text=f"参数：{BAUD_RATE}，8N1").grid(
            row=1, column=1, sticky="w", padx=(8, 0), pady=(7, 0)
        )

        auto_box = ttk.LabelFrame(outer, text="自动校准", padding=12)
        auto_box.pack(fill="x", pady=(14, 0))
        auto_box.columnconfigure(0, weight=1)
        ttk.Label(auto_box, textvariable=self.pc_time_var, style="Clock.TLabel").grid(
            row=0, column=0, sticky="w"
        )
        ttk.Button(
            auto_box,
            text="同步电脑当前时间",
            command=self.send_pc_time,
            style="Accent.TButton",
        ).grid(row=0, column=1, padx=(12, 0), ipady=6)

        manual_box = ttk.LabelFrame(outer, text="手动设置时间", padding=12)
        manual_box.pack(fill="x", pady=(14, 0))

        ttk.Label(manual_box, text="小时").grid(row=0, column=0)
        ttk.Spinbox(
            manual_box, from_=0, to=23, wrap=True, width=5,
            textvariable=self.hour_var, format="%02.0f"
        ).grid(row=1, column=0, padx=(0, 8), pady=(4, 0))
        ttk.Label(manual_box, text=":", font=("Consolas", 16, "bold")).grid(
            row=1, column=1
        )
        ttk.Label(manual_box, text="分钟").grid(row=0, column=2)
        ttk.Spinbox(
            manual_box, from_=0, to=59, wrap=True, width=5,
            textvariable=self.minute_var, format="%02.0f"
        ).grid(row=1, column=2, padx=8, pady=(4, 0))
        ttk.Label(manual_box, text=":", font=("Consolas", 16, "bold")).grid(
            row=1, column=3
        )
        ttk.Label(manual_box, text="秒").grid(row=0, column=4)
        ttk.Spinbox(
            manual_box, from_=0, to=59, wrap=True, width=5,
            textvariable=self.second_var, format="%02.0f"
        ).grid(row=1, column=4, padx=(8, 14), pady=(4, 0))
        ttk.Button(manual_box, text="填入当前时间", command=self.fill_pc_time).grid(
            row=1, column=5, padx=(0, 8)
        )
        ttk.Button(
            manual_box,
            text="发送手动时间",
            command=self.send_manual_time,
            style="Accent.TButton",
        ).grid(row=1, column=6)

        result_box = ttk.LabelFrame(outer, text="发送结果", padding=12)
        result_box.pack(fill="both", expand=True, pady=(14, 0))
        ttk.Label(result_box, textvariable=self.status_var, style="Status.TLabel").pack(
            anchor="w"
        )
        ttk.Label(
            result_box,
            textvariable=self.frame_var,
            font=("Consolas", 11),
            foreground="#285c8f",
        ).pack(anchor="w", pady=(8, 0))
        ttk.Label(
            result_box,
            text="提示：烧录软件或串口助手必须先关闭串口。",
            foreground="#666666",
        ).pack(anchor="w", side="bottom")

    def _update_pc_clock(self) -> None:
        self.pc_time_var.set(datetime.now().strftime("电脑时间  %Y-%m-%d  %H:%M:%S"))
        self.root.after(200, self._update_pc_clock)

    def _show_dependency_error(self) -> None:
        messagebox.showerror(
            "缺少依赖",
            "没有找到 pyserial。请在命令行执行：\n\n"
            "python -m pip install pyserial\n\n"
            "安装完成后重新运行本程序。",
        )

    def refresh_ports(self) -> None:
        if list_ports is None:
            self.port_combo["values"] = ()
            return

        ports = sorted(list_ports.comports(), key=lambda item: item.device)
        devices = [item.device for item in ports]
        previous = self.port_var.get()
        self.port_combo["values"] = devices

        if previous in devices:
            self.port_var.set(previous)
        elif devices:
            self.port_var.set(devices[0])
            details = next(item.description for item in ports if item.device == devices[0])
            self.status_var.set(f"已发现 {len(devices)} 个串口，当前：{devices[0]} ({details})")
        else:
            self.port_var.set("")
            self.status_var.set("未发现串口，请检查 USB 转串口模块")

    def fill_pc_time(self) -> None:
        now = datetime.now()
        self.hour_var.set(f"{now.hour:02d}")
        self.minute_var.set(f"{now.minute:02d}")
        self.second_var.set(f"{now.second:02d}")

    def send_pc_time(self) -> None:
        self._send_time(use_pc_clock=True)

    def send_manual_time(self) -> None:
        try:
            hour = int(self.hour_var.get())
            minute = int(self.minute_var.get())
            second = int(self.second_var.get())
            build_time_frame(hour, minute, second)
        except ValueError as exc:
            messagebox.showwarning("时间无效", str(exc))
            return
        self._send_time(use_pc_clock=False, manual_time=(hour, minute, second))

    def _send_time(
        self,
        *,
        use_pc_clock: bool,
        manual_time: tuple[int, int, int] | None = None,
    ) -> None:
        if serial is None:
            self._show_dependency_error()
            return

        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("未选择串口", "请先连接设备并选择串口。")
            return

        try:
            with serial.Serial(
                port=port,
                baudrate=BAUD_RATE,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.5,
                write_timeout=1.0,
            ) as connection:
                if use_pc_clock:
                    now = datetime.now()
                    hour, minute, second = now.hour, now.minute, now.second
                    mode_text = "自动校准"
                else:
                    assert manual_time is not None
                    hour, minute, second = manual_time
                    mode_text = "手动设置"

                frame = build_time_frame(hour, minute, second)
                connection.reset_output_buffer()
                written = connection.write(frame)
                connection.flush()

            if written != len(frame):
                raise serial.SerialTimeoutException(
                    f"只发送了 {written}/{len(frame)} 个字节"
                )

            hex_text = " ".join(f"{byte:02X}" for byte in frame)
            self.status_var.set(
                f"{mode_text}已发送：{hour:02d}:{minute:02d}:{second:02d} → {port}"
            )
            self.frame_var.set(f"发送帧：{hex_text}")
        except (serial.SerialException, OSError) as exc:
            self.status_var.set(f"发送失败：{exc}")
            messagebox.showerror(
                "串口发送失败",
                f"无法通过 {port} 发送时间：\n\n{exc}\n\n"
                "请确认串口未被烧录软件或其他串口工具占用。",
            )


def main() -> None:
    root = tk.Tk()
    SerialTimeSyncApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()

# mcu-digital-clock

基于 STC15W4K48S4 单片机的多功能数字钟。

This Keil C51 project now implements the five basic course requirements on the
ZJNU EELab V1.3 digital-clock board.

## Before programming

1. Open `Uart1_Demo.uvproj` in Keil uVision.
2. Replace only `STUDENT_ID_TEXT` in `config.h`. A four-digit ID
   is displayed steadily; a longer ID (for example, 12 digits) scrolls
   continuously. The length is calculated automatically.
3. Keep the STC clock setting at 11.0592 MHz so the timer and UART calculations
   match the firmware. This value was confirmed from the connected MCU's UART
   baud rate.
4. Build the project. The generated file is `Objects/Uart1_Demo.hex`.

The firmware drives P4.5 high in push-pull mode. This powers VCCD when jumper H2
is fitted between P4.5 and VCCD; the other H2 position connects VCCD directly to
+5 V.

## Project structure

- `main.c`: initialization and foreground scheduling only.
- `config.h`: student ID, startup time, alarm defaults and timing constants.
- `Application/app.c`: three display states and the complete edit workflow.
- `Drivers/board.c`: GPIO, display scan, clock timebase, alarm and buzzer.
- `Drivers/keys.c`: key debounce and long-press repeat.
- `Drivers/uart1.c`: UART1 initialization and time-frame parser.
- `chuankou/`: C# WinForms serial tool, source code and compiled EXE.

The Keil project shows the same layout as the `Application`, `Drivers` and
`Headers` source groups.

## Buttons

- After power-on the clock display is selected. Because the board has four
  digits, it alternates every three seconds between HH:MM and MM:SS.
- P1.3 in normal display mode: clock -> student ID -> alarm -> clock.
- P1.4 in clock/alarm mode: enter editing at the hour field.
- P1.4 while editing: decrement the flashing field.
- P1.5 while editing: increment the flashing field.
- P1.5 in the normal clock display: enable or disable the once-per-second
  ticking sound. The ticking sound is enabled by default.
- Hold P1.4 or P1.5 for 600 ms to start automatic repeat (every 120 ms).
- P1.3 while editing: confirm the field and move to the next one. Clock editing
  follows hour -> minute -> second -> finish; alarm editing follows hour ->
  minute -> finish.
- Any button stops a ringing alarm.

The display scan and clock timebase run in the 1 ms Timer0 interrupt, so button
handling does not stop or visibly block the display.

## UART1 time synchronization

- Port pins: RxD=P3.0, TxD=P3.1.
- Settings: 115200 baud, 8 data bits, no parity, 1 stop bit.
- Binary frame: `01 HH MM SS AA`.
- `HH`, `MM` and `SS` are packed BCD bytes.
- A valid frame is acknowledged with byte `06`.

Example: set the clock to 19:35:50 by sending these hexadecimal bytes:

```text
01 19 35 50 AA
```

Frames with an invalid length, BCD digit or time range are ignored.

## PC time synchronization tool

Run `chuankou\bin\SerialTimeSync.exe`, select the CH340 COM port and connect.
`同步电脑当前时间` sends the current PC clock; `发送手动时间` sends the values
entered in the three fields. The connection stays open, DTR/RTS remain disabled,
and the interface reports success only after receiving the MCU's `06` ACK.

## Default values

- Startup clock: 12:00:00.
- Alarm: 07:30, enabled.
- Alarm duration: 30 seconds, using a 500 Hz passive-buzzer waveform.
- Clock tick: a 40 ms, 500 Hz tone on each second while the clock display is
  selected. The alarm has priority over this short tone.

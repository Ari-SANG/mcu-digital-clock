# mcu-digital-clock

基于 STC15W4K48S4 单片机的多功能数字钟。

This Keil C51 project now implements the five basic course requirements on the
ZJNU EELab V1.3 digital-clock board.

## Before programming

1. Open `Uart1_Demo.uvproj` in Keil uVision.
2. The student number is configured as `STUDENT_ID_TEXT` in `config.h`. Its
   complete value scrolls, while its last four digits form the fixed display.
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
- `Application/app.c`: four display states and the complete edit workflow.
- `Drivers/board.c`: GPIO, Timer1 display scan, clock timebase, alarm and
  P1.0/ADC0 temperature measurement.
- `Drivers/music.c`: Timer0 hardware tone output, three built-in melodies,
  clock tick and alarm/preview sequencing.
- `Drivers/keys.c`: key debounce and long-press repeat.
- `Drivers/uart1.c`: UART1 initialization and time/alarm-frame parser.
- `chuankou/`: C# WinForms serial tool, source code and compiled EXE.

The Keil project shows the same layout as the `Application`, `Drivers` and
`Headers` source groups.

## Buttons

- After power-on the clock display is selected. Because the board has four
  digits, it alternates every three seconds between HH:MM and MM:SS.
- P1.3 in normal display mode: clock -> student ID -> alarm -> room
  temperature -> clock.
- P1.4 in clock/alarm mode: enter editing at the hour field.
- P1.4 while editing: decrement the flashing field.
- P1.5 while editing: increment the flashing field.
- P1.5 in the normal clock display: enable or disable the once-per-second
  ticking sound. The ticking sound is enabled by default.
- P1.5 in the student ID display: switch between scrolling `202436100135` and
  the fixed four-digit form `0135`. Power-on defaults to the scrolling form.
- P1.5 in the normal alarm display: cycle through alarm 1, alarm 2 and alarm 3.
- Hold P1.4 or P1.5 for 600 ms to start automatic repeat (every 120 ms).
- P1.3 while editing: confirm the field and move to the next one. Clock editing
  follows hour -> minute -> second -> finish; alarm editing follows hour ->
  minute -> music -> finish.
- The alarm music field is shown as `A1-2`, meaning alarm 1 uses music 2.
  P1.4/P1.5 selects the previous/next melody and restarts a six-second
  preview; P1.3 stops the preview and completes editing.
- Any button stops a ringing alarm.

The display scan and clock timebase run in the 1 ms Timer1 interrupt. Timer0's
hardware clock output drives P3.5 directly, so melody pitch does not depend on
interrupt latency and button handling does not block the display.

## Room temperature

The fourth display state reads the board's 10 kOhm, B=3950 NTC divider through
P1.0/ADC0. The 10-bit ADC result is converted to tenths of a degree Celsius for
normal indoor temperatures. For example, `261C` means 26.1 degrees Celsius; the
decimal point is illuminated after the second digit.

## UART1 time synchronization

- Port pins: RxD=P3.0, TxD=P3.1.
- Settings: 115200 baud, 8 data bits, no parity, 1 stop bit.
- Binary frame: `01 HH MM SS AA`.
- `HH`, `MM` and `SS` are packed BCD bytes.
- A valid frame is acknowledged with byte `06`.
- Alarm frame: `02 NN HH MM AA`. `NN` is the binary alarm number 01-03;
  `HH` and `MM` are packed BCD. A valid frame updates that alarm, selects its
  display and is acknowledged with byte `06`.
- Extended alarm frame: `03 NN HH MM MU AA`. `MU` is the binary melody number
  01-03. It updates the alarm time and melody in one acknowledged operation.

Example: set the clock to 19:35:50 by sending these hexadecimal bytes:

```text
01 19 35 50 AA
```

Frames with an unsupported command, invalid length or out-of-range time are
ignored. The supplied PC tool always generates valid packed-BCD fields.

## PC time synchronization tool

Run `chuankou\bin\SerialTimeSync.exe`, select the CH340 COM port and connect.
The `时间同步` page can send the current PC clock or a manually entered time.
The separate `闹钟设置` page can update the time and melody of any alarm. The
connection stays open, DTR/RTS remain disabled, and the interface reports
success only after receiving the MCU's `06` ACK.

## Default values

- Startup clock: 12:00:00.
- Three alarms: 08:00, 12:00 and 16:00. All three are active, and each can be
  selected and edited independently.
- Alarm 1 defaults to `Twinkle Twinkle Little Star`, alarm 2 to `Ode to Joy`,
  and alarm 3 to `Happy Birthday`. The selected melody repeats for up to 30
  seconds; any button stops it.
- The melodies use the C5-G6 range, song-specific timing and a 25 ms gap between
  notes for clear articulation on the board's passive buzzer.
- Alarm duration: up to 30 seconds, looping the selected passive-buzzer melody.
- Clock tick: a 40 ms, 500 Hz tone on each second while the clock display is
  selected. The alarm has priority over this short tone.

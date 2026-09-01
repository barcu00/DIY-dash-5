# BartzDash

Motorsport dashboard firmware for the **Waveshare ESP32-S3-Touch-LCD-5** non-B variant (800x480), prepared as a foundation for ECUMaster EMU Black CAN integration.

## BartzDash v0.1

Version 0.1 is a hardware/UI bring-up build. It intentionally uses **MOCK telemetry** and does **not** enable CAN yet.

Implemented in v0.1:

- Waveshare ESP32-S3-Touch-LCD-5 board profile
- 800x480 RGB LCD / ST7262
- GT911 capacitive touch
- 16 MB Flash configuration
- 8 MB OPI PSRAM configuration
- LVGL 8.4
- DASH / DIAG / TRACK screens
- touch navigation
- animated mock RPM, gear, speed, MAP, lambda, CLT, IAT, oil pressure, oil temperature, fuel pressure, battery voltage and TPS
- shift-light bar
- native mock-telemetry unit tests
- GitHub Actions firmware build
- single merged full-flash binary

## Build

Install PlatformIO and run:

```bash
pio test -e native
pio run -e waveshare_5
```

Build output is created in:

```text
.pio/build/waveshare_5/
```

Important files:

```text
firmware.bin
bootloader.bin
partitions.bin
BartzDash-v0.1-full.bin
```

GitHub Actions also publishes these files in the `BartzDash-v0.1-waveshare-5` artifact.

## Flashing - recommended method

For the first test, use the merged image. Put the board into download/bootloader mode and identify the serial port.

Erase the flash first:

```bash
python -m esptool --chip esp32s3 --port COM5 erase_flash
```

Then flash the complete image at offset `0x0`:

```bash
python -m esptool --chip esp32s3 --port COM5 --baud 921600 write_flash 0x0 BartzDash-v0.1-full.bin
```

Replace `COM5` with the actual device port. On Linux it will normally look like `/dev/ttyACM0` or `/dev/ttyUSB0`.

Alternatively, from a cloned repository PlatformIO can perform the upload directly:

```bash
pio run -e waveshare_5 -t upload --upload-port COM5
```

## Serial diagnostics

Serial monitor speed:

```text
115200 baud
```

Expected startup information includes the detected Flash/PSRAM size, LCD dimensions, touch status and UI startup status.

## Hardware acceptance test for v0.1

After flashing, verify:

1. the board boots without a reset loop;
2. the LCD shows the DASH screen in 800x480;
3. mock telemetry changes continuously;
4. the shift bar follows RPM;
5. touch switches DASH -> DIAG -> TRACK -> DASH;
6. DIAG reports display/touch/PSRAM information;
7. CAN is shown as `DISABLED IN v0.1`;
8. the interface remains responsive for at least 15 minutes.

A successful GitHub Actions build proves compilation only. LCD, GT911 and runtime stability still require this test on the physical board.

## Next milestone - v0.2

Planned ECUMaster integration:

- ESP32 TWAI / CAN
- TX GPIO15
- RX GPIO16
- selectable 500 kbit/s / 1 Mbit/s
- ECUMaster EMU Black base ID 0x600
- selectable MOCK / ECUMASTER data source
- CAN ONLINE/OFFLINE timeout
- RPM, TPS, MAP, lambda, CLT, IAT, oil pressure, oil temperature, fuel pressure, battery, gear, speed and other available EMU stream values

## Project documentation

Design specification:

```text
docs/superpowers/specs/2026-09-01-bartzdash-v0.1-design.md
```

Implementation plan:

```text
docs/superpowers/plans/2026-09-01-bartzdash-v0.1.md
```

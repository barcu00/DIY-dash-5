# RaceChrono Bluetooth LE setup

DIY Dash 5 can act as a RaceChrono DIY Monitor device over Bluetooth LE. The
integration adds lap timing, delta, sector, GPS, satellite, accuracy, speed,
position, and acceleration channels to the normal tile picker. Engine telemetry
is not replaced: **CAN / DEMO remains active** while RaceChrono supplies the
additional channels.

## Enable the dashboard

1. Open `SETTINGS > DATA & CAN > RACECHRONO`.
2. On the `CONNECTION` page, enable the switch.
3. Leave the screen with `< BACK`, `DASH`, or `TRACK` to save the setting.
4. The status changes to `ADVERTISING`. This is normal before RaceChrono starts
   a session and initiates the connection.

`RESTART BLE` disconnects the current client and immediately resumes
advertising. It does not restart CAN, DEMO, the display, or the shift light.

## Add DIY Dash in RaceChrono

Perform this one-time setup in the RaceChrono phone app:

```text
RaceChrono Settings
  Add other device
  RaceChrono DIY
  Bluetooth LE
  DIY DASH RC
  Monitor
```

Select the resulting device for the session. RaceChrono is the BLE central and
connects to `DIY DASH RC` after the session begins; pairing from the phone's
system Bluetooth page is not required.

The firmware implements the RaceChrono DIY Monitor service `0x1FF8`, with
configuration characteristic `0x0005` and values characteristic `0x0006`.
Configuration is exchanged automatically after RaceChrono subscribes.

## Dashboard pages

The `CONNECTION` page shows the BLE state, last-packet age, configured and active
channel counts, satellites, GPS accuracy, and fix quality. The `CHANNELS` page
lists six channels at a time and filters them by `ALL`, `ACTIVE`, `NO DATA`, or
`ERROR`. The 33-channel catalog occupies six pages; the final page has three
rows. Unavailable or stale values show `---`.

`THEORETICAL / REFERENCE` follows the comparison lap selected by RaceChrono.
It is not independently calculated by DIY Dash.

| Connection | Channels | Tile parameter picker |
| --- | --- | --- |
| ![RaceChrono connection](../ui/screenshots/racechrono-connection.png) | ![RaceChrono channels](../ui/screenshots/racechrono-channels.png) | ![RaceChrono picker](../ui/screenshots/racechrono-picker.png) |

## Add a RaceChrono tile

1. Hold a visible tile, or open a hidden slot in `SETTINGS > LAYOUTS`.
2. Choose `SELECT PARAMETER`.
3. Open the `RACECHRONO` category.
4. Select a channel and save the tile.

RaceChrono entries remain selectable while BLE is disabled or disconnected.
This preserves the layout; the tile displays `---` until valid data arrives.
Warnings use the same per-tile threshold, reset, delay, modal, and buzzer flow as
other numeric parameters.

## Test firmware

GitHub Actions packages the ready-to-flash image as:

```text
DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin
```

Flash it at address `0x0` using the instructions in the repository README. The
software checks cover protocol framing, queue limits, session recovery, UI
redraws, ESP32-S3 compilation, and firmware packaging. Physical phone, BLE
range, reconnect, heap, CAN-load, and long-duration tests must still be
performed on the target board before a release is considered hardware-validated.

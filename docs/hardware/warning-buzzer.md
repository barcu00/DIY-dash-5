# Warning buzzer (development firmware)

Target: Waveshare ESP32-S3-Touch-LCD-5 non-B. Use an **active 12 V buzzer**
with its own oscillator, not a passive piezo requiring an audio-frequency signal.
This feature is on dashboard-dev; the older published v0.2.1 image does not include it.

## Wiring

| Connection | Destination |
| --- | --- |
| Fused external +12 V | Buzzer positive |
| Buzzer negative | Board DO0 / DOUT0 |
| External supply negative | Board GND |

DO0 switches the low side; it does not supply 12 V. Do not connect buzzer positive
to DO0 or use DI_COM as the output return. Keep load current below the board's
450 mA/channel maximum and within the buzzer rating. Use a suitably protected
automotive supply; if the chosen sounder is inductive, follow its manufacturer's
suppression requirements. Disconnect power before wiring.

DO0 is CH422G **OC0**, logical expander pin **8**, not ESP32 GPIO8. The optocoupler
LED makes the internal control active LOW. The code uses the board-owned expander
and modifies this output only; no extra I2C initialization or whole-port writes.

Sources: [Waveshare interface description](https://docs.waveshare.com/ESP32-S3-Touch-LCD-5#interface-description),
[board schematic](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-5/ESP32-S3-Touch-LCD-5-Sch.pdf).

## Behavior

- One 200 ms test chirp after successful UI/board initialization, at each reboot.
- Timing starts at the first ON request, so initial rendering does not consume
  the chirp. A 450 ms quiet gap separates it from any warning already active.
- Warning sound: 150 ms ON, 450 ms OFF while any tile warning is unacknowledged.
- ACKNOWLEDGE silences that alarm. Other unacknowledged alarms still sound;
  an alarm that clears and then breaches again sounds again.
- SETTINGS > SYSTEM > WARNING SOUND disables warning audio only. Startup test
  remains enabled, as requested. Changes apply immediately and save on screen exit.
- Works with DEMO and CAN, including while navigating SETTINGS or editing tiles.
- Timing uses millis, not delay/tone/PWM. Only output transitions trigger I2C writes.
- Schema 6 settings migrate to schema 7 with all preset banks retained and warning
  sound enabled by default. Older schemas retain their existing migration paths.

Migration preserves settings only when existing NVS is retained. Flashing the
complete full BIN at 0x0 overwrites the flash image, including the NVS region;
back up existing flash/settings before doing so.

## Board validation still required

GitHub tests exercise timing, rollover, real warning acknowledgement and saved
configuration; firmware compilation checks the driver API. They cannot confirm
physical sound, DO0 polarity on a different PCB revision, bootloader/reset output
transients or electrical wiring. Verify these on the intended board before road use.
The startup chirp tests the connected sounder, not a feedback-monitored circuit.

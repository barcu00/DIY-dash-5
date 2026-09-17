# Warning buzzer implementation plan

> Execute inline using executing-plans in the existing user-selected checkout.

**Goal:** Drive an active 12 V buzzer on DO0 with one startup chirp and acknowledged-warning-aware pulses.

**Architecture:** A platform-independent BuzzerModel returns the desired state without sleeps. BoardDisplay writes only output transitions to CH422G OC0 (logical pin 8, active low through the board optocoupler). App consumes unacknowledged warnings under the UI lock. Append WARNING SOUND to configuration and migrate schema 6 without losing banks.

**Tech stack:** C++17, ESP32-S3, CH422G, Unity, LVGL 8.4, GitHub Actions.

**Spec:** User-approved conversation: active 12 V buzzer on DO0; single startup test; intermittent warning; ACK silences that warning, a new warning sounds; WARNING SOUND saved on exit. Sensor input excluded.

## Constraints

- Startup test always runs once after successful initialization: 200 ms, timed
  from its first ON request, then 450 ms quiet before any warning pulse.
- Warning cadence: 150 ms on / 450 ms off; any unacknowledged warning requests sound.
- WARNING SOUND disables warning audio only, not the requested startup test.
- No new I2C bus, whole-port output writes, PWM or blocking tone delays. DO1 and LCD/touch IO retained.
- Tests/builds on GitHub only; no main merge or release.

## Tasks

- [x] Add test/test_buzzer/test_main.cpp to exercise startup boundaries, pulses, disabling, ACK/rearm and uint32 rollover. Run GitHub RED before implementing src/alarms/buzzer_model.h/.cpp.
- [x] Add WARNING SOUND to AppConfig (schema 7), legacy schema-6 migration and test preservation/round trip. Run GitHub RED before migration implementation.
- [x] Connect BoardDisplay beginBuzzer/setBuzzer through existing expander, initialize OFF, cache successful writes and retry failures. Connect App after UI startup and inside loop LVGL lock; use warnings.nextModal().has_value() for unacknowledged warnings.
- [x] Add SYSTEM checkbox and stageSettings(candidate,false), retaining exit-save. Document wiring (+12 V to buzzer+, buzzer- to DO0, supply- to board GND, not DI_COM), external power and hardware test limits.
- [x] GitHub GREEN tests, firmware build and production-view render; read-only review; download full BIN and record tested commit/hash.

## Verification record

- Tested source commit: `a001a97cc24d8479201b83e809fa4e3cb1f032a5`.
- RED: runs `35233487612` (missing buzzer), `35233583315` (schema-6 migration), `35234077333` (delayed startup and merged warning tone).
- [Firmware/test run](https://github.com/barcu00/DIY-dash-5/actions/runs/35234247581): success; 194 native tests, 24 Python tests, ESP32-S3 firmware and full-image packaging.
- [Production LVGL render](https://github.com/barcu00/DIY-dash-5/actions/runs/35234247734): success; existing 14 production-view scenes preserved.
- Independent read-only review found the startup timing edge; after regression tests and the fix, no blocking regression remained.
- Full BIN: 16777216 bytes, flash offset `0x0`; SHA-256 `7098F88B021270D094755B83F79AAF9C044B525A46AB7AAEDC838AB16C5DA44E`.
- Downloaded workspace artifact: `artifacts/firmware-buzzer-a001a97/DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`.
- Linker-reported static internal RAM: 142488/327680 bytes (43.5%). Not a runtime heap/PSRAM measurement.
- No local tests/builds; physical buzzer/wiring/reset behavior still requires a board test. Branch remains dashboard-dev; no main merge or new release.

## Test contract

```cpp
BuzzerModel buzzer;
buzzer.begin(1000);
assert(buzzer.update(1000, false, false));
assert(!buzzer.update(1200, false, false));
assert(!buzzer.update(1300, true, true)); // startup quiet gap
assert(buzzer.update(1650, true, true));
assert(!buzzer.update(1800, true, true));
assert(!buzzer.update(1850, true, false)); // acknowledged
assert(buzzer.update(1851, true, true));  // new alarm
```

# Warning buzzer implementation plan

> Execute inline using executing-plans in the existing user-selected checkout.

**Goal:** Drive an active 12 V buzzer on DO0 with one startup chirp and acknowledged-warning-aware pulses.

**Architecture:** A platform-independent BuzzerModel returns the desired state without sleeps. BoardDisplay writes only output transitions to CH422G OC0 (logical pin 8, active low through the board optocoupler). App consumes unacknowledged warnings under the UI lock. Append WARNING SOUND to configuration and migrate schema 6 without losing banks.

**Tech stack:** C++17, ESP32-S3, CH422G, Unity, LVGL 8.4, GitHub Actions.

**Spec:** User-approved conversation: active 12 V buzzer on DO0; single startup test; intermittent warning; ACK silences that warning, a new warning sounds; WARNING SOUND saved on exit. Sensor input excluded.

## Constraints

- Startup test always runs once after successful initialization: 200 ms.
- Warning cadence: 150 ms on / 450 ms off; any unacknowledged warning requests sound.
- WARNING SOUND disables warning audio only, not the requested startup test.
- No new I2C bus, whole-port output writes, PWM or blocking tone delays. DO1 and LCD/touch IO retained.
- Tests/builds on GitHub only; no main merge or release.

## Tasks

- [ ] Add test/test_buzzer/test_main.cpp to exercise startup boundaries, pulses, disabling, ACK/rearm and uint32 rollover. Run GitHub RED before implementing src/alarms/buzzer_model.h/.cpp.
- [ ] Add WARNING SOUND to AppConfig (schema 7), legacy schema-6 migration and test preservation/round trip. Run GitHub RED before migration implementation.
- [ ] Connect BoardDisplay beginBuzzer/setBuzzer through existing expander, initialize OFF, cache successful writes and retry failures. Connect App after UI startup and inside loop LVGL lock; use warnings.nextModal().has_value() for unacknowledged warnings.
- [ ] Add SYSTEM checkbox and stageSettings(candidate,false), retaining exit-save. Document wiring (+12 V to buzzer+, buzzer- to DO0, supply- to board GND, not DI_COM), external power and hardware test limits.
- [ ] GitHub GREEN tests, firmware build and production-view render; read-only review; download full BIN and record tested commit/hash.

## Test contract

```cpp
BuzzerModel buzzer;
buzzer.begin(1000);
assert(buzzer.update(1000, false, false));
assert(!buzzer.update(1200, false, false));
assert(buzzer.update(1300, true, true));
assert(!buzzer.update(1450, true, true));
assert(!buzzer.update(1500, true, false)); // acknowledged
assert(buzzer.update(1501, true, true));  // new alarm
```

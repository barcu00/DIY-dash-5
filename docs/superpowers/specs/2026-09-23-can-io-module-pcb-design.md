# CAN I/O module PCB design

Approved conversational scope captured on 2026-09-23. This document defines the hardware and firmware boundary for a low-cost external automotive sensor module used with DIY Dash 5.

## Purpose

The module expands the dashboard with protected external sensor inputs, a Type-K EGT input, Flex Fuel decoding, two relay drivers, two configurable 0–5 V outputs, BMW MS43 K-line acquisition, and CAN transport. The dashboard is the configuration interface. After configuration, the module stores its settings and operates independently.

The target is a low-cost, serviceable design for enthusiast and track use. It is not a safety controller and must not be used to control braking, steering, throttle, airbags, or other safety-critical functions.

## Mechanical target

- Use the user-selected 24-pin plastic ECU enclosure and its right-angle three-row PCB header.
- Reference enclosure dimensions from the supplied drawings: approximately 117.4 mm overall width, 114.65 mm overall length, 39.2 mm body height, and 101.7 mm main body width.
- Use a custom KiCad footprint derived from the supplied connector drawings. The connector has 24 contacts in three rows, with approximately 3.33 mm horizontal pitch.
- Use a 2-layer, 1.6 mm FR-4 PCB with 1 oz copper to minimize prototype and production cost.
- Place the PCB header on the short edge of the PCB and keep all high-profile components below the enclosure height limit.
- Use the enclosure's PCB guides and mounting features. Do not release manufacturing Gerbers until one physical enclosure and connector have been measured and a paper or 3D-printed fit template has been checked.
- The enclosure is treated as splash-resistant only until the assembled connector, seals, lid gasket, and cable seals pass a physical ingress test.

## External connector pinout

| Pin | Signal | Direction | Notes |
| ---: | --- | --- | --- |
| 1 | VBAT | Input | Fused vehicle 12 V supply |
| 2 | POWER_GND | Power | Main power return |
| 3 | IGN | Input | Ignition/wake input |
| 4 | CAN_H | Bidirectional | Main module/dashboard CAN |
| 5 | CAN_L | Bidirectional | Main module/dashboard CAN |
| 6 | K_LINE | Bidirectional | BMW MS43 DS2/K-line |
| 7 | SENSOR_5V | Output | Protected sensor supply |
| 8 | SENSOR_GND | Power | Sensor reference ground |
| 9 | AIN1 | Input | Universal analog/resistive input |
| 10 | AIN2 | Input | Universal analog/resistive input |
| 11 | AIN3 | Input | Universal analog/resistive input |
| 12 | AIN4 | Input | Universal analog/resistive input |
| 13 | AIN5 | Input | Universal analog/resistive input |
| 14 | AIN6 | Input | Universal analog/resistive input |
| 15 | AIN7 | Input | Universal analog/resistive input |
| 16 | AIN8 | Input | Universal analog/resistive input |
| 17 | EGT_K_POS | Input | Type-K thermocouple positive |
| 18 | EGT_K_NEG | Input | Type-K thermocouple negative |
| 19 | RELAY_OUT1 | Output | Protected low-side relay-coil driver |
| 20 | RELAY_OUT2 | Output | Protected low-side relay-coil driver |
| 21 | FLEX_IN | Input | Flex Fuel frequency/pulse-width signal |
| 22 | AOUT1 | Output | Configurable 0–5 V signal output |
| 23 | AOUT2 | Output | Configurable 0–5 V signal output |
| 24 | SERVICE | Bidirectional | Factory test/service signal; no normal harness connection |

Flex Fuel sensor power is supplied by the vehicle harness, not through the module. Pin 24 is reserved for manufacturing test and recovery so the production connector count remains sufficient.

## Processing and memory

- Use STM32G0B1CBT6 in LQFP48.
- Use the internal 12-bit ADC for the eight universal inputs and supply monitoring.
- Use both internal 12-bit DAC channels for AOUT1 and AOUT2.
- Use hardware timers for Flex Fuel capture and non-blocking scheduling.
- Use internal flash for configuration. Store two versioned copies with sequence number and CRC; commit a new copy before invalidating the previous one.
- Expose SWD pads, NRST, 3V3, and GND as unpopulated test points.

## Power architecture

- Accept nominal 12 V vehicle power on VBAT.
- Protect VBAT with a replaceable harness fuse, reverse-polarity MOSFET, surge TVS, input LC filtering, and a 60 V-capable buck stage.
- Use LMR16006-Q1 or a pin-compatible qualified alternative for the 12 V to 5 V rail. The 600 mA rating is sufficient for the controller, interfaces, analog outputs, and the protected external sensor supply.
- Use a 3.3 V low-noise LDO rated for at least 300 mA for the MCU and analog logic.
- Protect SENSOR_5V with a resettable fuse or current-limited load switch. Target a 250 mA maximum external load.
- Measure VBAT, 5 V, and 3.3 V rails with the ADC. Resistance calculations use the measured 5 V rail rather than assuming an ideal supply.
- IGN uses a protected divider and Schmitt input. Firmware supports delayed shutdown so pending configuration can finish saving.

## Universal analog inputs

All AIN1–AIN8 have identical hardware and are individually configurable from the dashboard.

Each connector input uses:

- approximately 10 kOhm total series resistance,
- a 15 kOhm lower divider resistor so 5 V remains below the 3.3 V ADC limit,
- a local RC low-pass filter,
- low-leakage clamps to the analog rails,
- ESD protection at the connector,
- open-wire and short-circuit diagnostics appropriate to the selected mode.

Supported modes:

1. **Voltage 0–5 V**: pull-up disabled; arbitrary two-point or multi-point conversion.
2. **Voltage 0.5–4.5 V**: pull-up disabled; detects below-range and above-range faults.
3. **Resistance low range**: switched 1 kOhm pull-up to regulated 5 V for fuel level and low-resistance pressure senders.
4. **NTC**: switched 4.7 kOhm pull-up to regulated 5 V for automotive temperature sensors.
5. **Read-only tap**: pull-up disabled and long averaging for observing an existing ECU/gauge signal without intentionally biasing it.

Use two inexpensive 74HCT4051 multiplexers, one for each pull-up value. Only the selected channel and selected pull-up are enabled during a measurement. Both multiplexers default disabled through hardware pull resistors during reset. Firmware never enables a pull-up on a channel configured for an active voltage sensor or read-only tap.

Calibration supports voltage/value points, resistance/value points, NTC Beta or Steinhart-Hart coefficients, fuel Empty/Full calibration, optional multi-point tank shape correction, inversion, filtering, and engineering units.

## Type-K EGT input

- Use MAX31855KASA+ as the low-cost Type-K interface. Do not use MAX6675 because its upper range is too restrictive for engine EGT.
- Route EGT_K_POS and EGT_K_NEG as a symmetric differential pair directly from connector pins 17 and 18 to the converter.
- Place the converter immediately behind the connector and keep copper areas and heat sources symmetric around both thermocouple paths.
- Use Type-K extension wire up to the harness connector. Standard copper-alloy connector contacts introduce additional junctions; keeping the two transitions isothermal minimizes their error. The module provides a user calibration offset for residual installation error.
- Provide open-thermocouple and short/fault reporting over CAN.

## Flex Fuel input

- FLEX_IN supports the common Continental/GM-style frequency and pulse-width signal.
- Use a protected Schmitt-trigger front end with a 5 V pull-up, series limiting, and overvoltage clamps.
- Decode ethanol content from frequency and fuel temperature from pulse width using an MCU timer capture channel.
- Detect missing, stuck, below-range, and above-range signals.

## Analog outputs

- AOUT1 and AOUT2 are signal outputs for high-impedance ECU or logger inputs, not power outputs.
- Generate both channels with the STM32's internal 12-bit DAC.
- Use a 5 V rail-to-rail dual operational amplifier such as TLV9002-Q1. Configure each channel as a non-inverting gain stage with enough gain margin to calibrate the DAC range to 0–5 V.
- Add output isolation resistance, RC filtering, ESD protection, and short-current limiting.
- Target load impedance is at least 10 kOhm. Accuracy target after calibration is within 1% of full scale over the normal operating temperature range.
- Firmware maps any available module or received CAN channel to either output using configurable input minimum, input maximum, output minimum, output maximum, inversion, clamp behavior, timeout behavior, and a safe startup/fault voltage. The default startup and timeout output is 0 V.

## Relay outputs

- RELAY_OUT1 and RELAY_OUT2 are protected low-side drivers for external 12 V relay coils or an active buzzer.
- Use 60 V logic-level N-channel MOSFETs, gate resistors and pull-downs, connector-side ESD protection, and flyback suppression.
- Limit the documented continuous load to 500 mA per channel and verify thermal rise during validation.
- Outputs remain off during reset and boot. Firmware supports manual control, threshold control, hysteresis, delay, and fail-safe off.

## CAN interface and configuration

- Use one high-speed CAN 2.0/FD-capable transceiver with 3.3 V logic compatibility, such as TJA1051T/3 or an equivalent low-cost qualified part.
- Populate switchable 120 Ohm termination using a jumper or solder bridge; default unpopulated/open.
- Use the same CAN connection for live telemetry and configuration. Default bitrate is 500 kbit/s and is recoverable to a fixed service bitrate through the SERVICE pin at boot.
- Use configurable base identifiers and node ID to avoid collisions with vehicle networks.
- The dashboard discovers modules, reads capabilities and current configuration, edits settings, validates them, writes a complete transaction, receives an acknowledgement, and reads the saved configuration back for verification.
- Configuration writes require an unlock token, transaction sequence number, payload CRC, and explicit COMMIT. Partial or invalid transactions never replace the active configuration.
- Configuration includes all input modes and curves, filters, channel names and units, Flex Fuel settings, EGT offset, relay logic, analog-output mappings, CAN IDs, bitrate, update rates, node ID, and safe-state behavior.
- Normal live data continues without dashboard presence. A dashboard is required only to change settings or view diagnostics.
- Do not enable configuration writes while engine speed is non-zero unless the user explicitly enters service mode.

## BMW MS43 K-line gateway

- Use ST L9613 for K-line physical interfacing because the module must support both stock DS2 operation and the previously discussed increased-rate patched stream.
- Support stock 9.6 kbit/s DS2 and patched operation up to 125 kbit/s.
- The module acts as an active tester when polling is required; it is not described as a passive decoder.
- Provide a software-controlled high-impedance/off state so normal diagnostics and flashing tools can take control of the K-line.
- K-line data selected by the user is decoded and published on CAN using the same channel model as local sensors.

## PCB placement and routing

- Divide the PCB into connector/protection, power, digital/CAN/K-line, universal analog, EGT, and output zones.
- Keep the buck converter, relay drains, K-line protection, and CAN common-mode currents away from the EGT and ADC regions.
- Use the bottom layer as a substantially continuous ground plane. Do not split ground under signal paths; control return current through placement.
- Join power and sensor grounds at a controlled low-impedance region near the supply entry while keeping thermocouple and ADC return currents away from relay and buck currents.
- Route CAN as a tightly coupled pair with short, symmetric connection to the transceiver and termination option.
- Keep EGT traces short, symmetric, free of vias where possible, and away from copper pours that create thermal gradients.
- Place protection components closest to connector pins, then filtering, then conversion circuitry.
- Add labeled test points for all rails, CAN TX/RX, K-line TX/RX, EGT SPI, Flex capture, both DAC nodes, both analog outputs, and each relay gate.

## Firmware behavior

- Sample active voltage channels at 100 Hz and publish filtered values at configurable rates up to 50 Hz.
- Sample resistive channels sequentially: enable the selected pull-up, wait for settling, acquire multiple ADC samples, disable the pull-up, then calculate resistance using measured 5 V.
- Apply per-channel median/outlier rejection followed by configurable low-pass filtering.
- Publish raw voltage/resistance, converted value, and diagnostic state separately.
- Keep acquisition, CAN transmission, K-line polling, relay logic, and configuration storage non-blocking.
- Use a watchdog and place all outputs into their configured safe state after internal faults or data timeouts.

## Verification and release gates

1. Run KiCad ERC with no unexplained errors.
2. Run KiCad DRC with no unexplained errors and verify connector courtyard, enclosure keep-outs, and creepage around VBAT.
3. Review the custom connector footprint against the supplied drawing and a printed 1:1 template.
4. Measure a physical enclosure and connector before finalizing board outline and manufacturing files.
5. Bench-test reverse polarity, supply ramping, brownout, reset behavior, and current consumption.
6. Apply controlled automotive transient tests appropriate to the selected TVS and buck limits; do not connect to a vehicle before these pass.
7. Calibrate all eight inputs with precision voltage and resistance standards and verify every pull-up mode.
8. Verify open/short diagnostics and confirm no pull-up appears during reset.
9. Calibrate both 0–5 V outputs at several loads of 10 kOhm or greater and verify safe startup and timeout voltage.
10. Test EGT against a known thermocouple simulator at ambient and elevated connector temperatures.
11. Test CAN configuration interruption, CRC rejection, duplicate frames, rollback, read-back, bus-off recovery, and identifier collision handling.
12. Test K-line disable/high-impedance behavior before using diagnostic or flashing tools.
13. Test both relay channels at 500 mA with representative relay coils and verify flyback and thermal behavior.
14. Perform an assembled enclosure fit test, harness retention test, vibration check, and ingress test.

## Deliverables after specification approval

- KiCad project with hierarchical schematic sheets.
- Custom symbol and footprint for the supplied 24-pin ECU connector.
- PCB layout matched to the verified enclosure sample.
- BOM with preferred low-cost parts and qualified substitutions.
- Pick-and-place, fabrication drawings, Gerbers, drill files, and assembly drawings only after the physical-fit gate.
- STM32 module firmware and dashboard-side configuration protocol/UI integration.
- Bring-up checklist, pinout document, and calibration procedure.

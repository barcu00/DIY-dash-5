# CAN I/O module electrical rebuild implementation plan

**Goal:** Replace the descriptive placeholder schematics and arbitrary PCB mappings with a manufacturer-pin-accurate, two-layer KiCad project validated on GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-24-can-io-electrical-rebuild-design.md`

## 1. Make incomplete electrical data fail

- Extend the hardware validator and unit tests to require real schematic symbols, pins, wires/labels, component counts, SWD 1x5 mapping, exact critical-IC pin maps, and schematic/PCB footprint parity.
- Add regression tests for the missing four-terminal CAN choke and for MCU power/ground pins.
- Prove the current project fails these tests before changing the hardware sources.

## 2. Establish verified pin assignments

- Add a machine-readable pin-map manifest for STM32G0B1CBT6, LMR16006Y-Q1, TPS7A2033, TPS22945, CD74HCT4051, L9613, TJA1051T/3, TLV9002-Q1, MAX31855KASA+, and 74LVC1G17.
- Record manufacturer data-sheet URLs and revisions.
- Verify GPIO alternate functions, ADC channels, DAC channels, timer capture, FDCAN, UART, SPI, SWD, power pins, and reset/boot pins.
- Make unit tests reject duplicate MCU pins, missing supply pins, or unsupported peripheral assignments.

## 3. Generate real hierarchical schematics

- Replace the root and five functional sheets with valid KiCad 9 symbols and electrical connectivity.
- Add the 1x5 2.54 mm SWD connector and visible pin-1 marking.
- Complete all required passive networks, decoupling, protection, regulator feedback, and safe-state biasing.
- Run the structural tests locally; push and use GitHub KiCad ERC as the authoritative parser/electrical check.

## 4. Correct BOM and footprints

- Reconcile every schematic reference with the BOM.
- Replace generic component blocks with package-accurate KiCad footprints.
- Implement L2 as a true four-terminal common-mode choke plus fitted bypass option.
- Keep the custom 24-pin connector explicitly provisional pending sample measurement.

## 5. Regenerate and route the two-layer PCB

- Update the PCB from the verified netlist rather than hand-maintaining arbitrary pad mappings.
- Re-place functional zones and route critical EGT, CAN, power, relay, analog, and digital nets in that order.
- Preserve a substantially continuous bottom ground plane and add accessible SWD routing.
- Require zero unrouted items and exact schematic parity.

## 6. Validate on GitHub and publish review artifacts

- Run the complete Python, ERC, parity, DRC, and export workflow on GitHub.
- Inspect reports and rendered schematic/PCB artifacts, fix every unexplained issue, and rerun until clean.
- Update the README and fabrication notice with the corrected maturity status and SWD pinout.
- Do not mark the design fabrication-ready until the physical connector/enclosure fit gate is completed.


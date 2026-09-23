# CAN I/O Module Hardware Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a complete, reviewable KiCad hardware project for the 24-pin CAN I/O module, including hierarchical schematics, custom connector library, routed two-layer PCB, BOM, prototype fabrication outputs, and GitHub-hosted ERC/DRC validation.

**Architecture:** The board is divided into power, MCU, universal analog-input, communications/EGT, and output sheets that meet at a single 24-pin connector. STM32G0B1CBT6 handles acquisition and transport; L9613 is the fixed K-line transceiver, and all eight analog channels support pull-up off, 1 kOhm to 5 V, or 4.7 kOhm to 5 V through multiplexed excitation. A checked-in Python contract test validates safety-critical connectivity before KiCad CLI performs ERC, DRC, PDF plotting, BOM export, and prototype Gerber generation on GitHub Actions.

**Tech Stack:** KiCad 9 native files, Python 3 standard-library tests, KiCad CLI in GitHub Actions, Markdown/CSV manufacturing documentation.

**Spec:** `docs/superpowers/specs/2026-09-23-can-io-module-pcb-design.md`

## Global Constraints

- Use the user-selected 24-pin plastic ECU enclosure and right-angle three-row PCB header.
- Use STM32G0B1CBT6, L9613 only, eight identical universal inputs, MAX31855KASA+, two 0–5 V signal outputs, two 500 mA low-side outputs, Flex Fuel input, and one configurable CAN interface.
- Pull-ups are software selectable per input: OFF, 1 kOhm to regulated 5 V, or 4.7 kOhm to regulated 5 V.
- PCB is 2-layer, 1.6 mm FR-4, 1 oz copper; prototype manufacturing outputs are not production-approved until a physical enclosure and connector pass the fit gate.
- Hardware checks, ERC, DRC, exports, and fabrication generation run on GitHub Actions; do not claim local KiCad validation.
- Preserve unrelated repository changes and add only the module hardware project, validation, workflow, and documentation.

## Review Focus

- **Reset pull-up safety:** every 74HCT4051 enable input must have a hardware default that disables excitation while the MCU is reset.
- **Five-volt input safety:** each 0–5 V input must divide to no more than 3.1 V at the MCU ADC pin and must retain connector-side ESD protection.
- **Thermocouple accuracy:** EGT paths must remain symmetric, short, and separated from the buck converter, relay drains, and copper heat sources.
- **Output startup safety:** relay gates and both analog outputs must default to zero/off before firmware configures the MCU.
- **Automotive transients:** VBAT, IGN, CAN, K-line, Flex Fuel, analog inputs, analog outputs, and relay outputs must have explicit connector-side protection and no unprotected path into the MCU.

---

## File structure

The implementation creates this focused tree:

```text
hardware/can-io-module/
  can-io-module.kicad_pro
  can-io-module.kicad_sch
  can-io-module.kicad_pcb
  fp-lib-table
  sym-lib-table
  sheets/
    power.kicad_sch
    mcu.kicad_sch
    analog-inputs.kicad_sch
    communications-egt.kicad_sch
    outputs.kicad_sch
  lib/
    diy_dash_can_io.kicad_sym
    diy_dash_can_io.pretty/
      ECU_FCI_24P_RightAngle.kicad_mod
      ECU_CASE_BOARD_GUIDE.kicad_mod
  bom/
    can-io-module-bom.csv
    approved-alternates.csv
  manufacturing/
    README.md
    prototype-fabrication-notice.txt
  README.md
  bringup-checklist.md
scripts/
  check_can_io_hardware.py
  export_can_io_hardware.ps1
test/hardware/
  test_can_io_hardware.py
.github/workflows/
  can-io-hardware.yml
```

Each schematic sheet owns one electrical responsibility. The PCB consumes the shared net names from those sheets. The test script treats the KiCad files and BOM as one design contract and rejects missing protection, wrong pin mapping, absent safe-state resistors, or an invalid board outline.

### Task 1: Hardware contract and CI scaffold

**Files:**
- Create: `test/hardware/test_can_io_hardware.py`
- Create: `scripts/check_can_io_hardware.py`
- Create: `.github/workflows/can-io-hardware.yml`
- Create: `hardware/can-io-module/README.md`
- Create: `hardware/can-io-module/manufacturing/prototype-fabrication-notice.txt`

**Interfaces:**
- Consumes: the approved spec at `docs/superpowers/specs/2026-09-23-can-io-module-pcb-design.md`.
- Produces: `validate_project(root: pathlib.Path) -> list[str]`, whose empty result means the checked-in hardware contract passes; GitHub job `can-io-hardware` running Python tests and KiCad CLI.

- [ ] **Step 1: Write the failing hardware contract test**

```python
from pathlib import Path
import unittest

from scripts.check_can_io_hardware import validate_project


class CanIoHardwareContractTest(unittest.TestCase):
    def test_complete_project_contract(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_project(root))


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL because `scripts.check_can_io_hardware` does not exist.

- [ ] **Step 3: Implement the project-contract validator**

Create `scripts/check_can_io_hardware.py` with:

```python
from __future__ import annotations

import csv
from pathlib import Path


REQUIRED_FILES = (
    "hardware/can-io-module/can-io-module.kicad_pro",
    "hardware/can-io-module/can-io-module.kicad_sch",
    "hardware/can-io-module/can-io-module.kicad_pcb",
    "hardware/can-io-module/sheets/power.kicad_sch",
    "hardware/can-io-module/sheets/mcu.kicad_sch",
    "hardware/can-io-module/sheets/analog-inputs.kicad_sch",
    "hardware/can-io-module/sheets/communications-egt.kicad_sch",
    "hardware/can-io-module/sheets/outputs.kicad_sch",
    "hardware/can-io-module/lib/diy_dash_can_io.kicad_sym",
    "hardware/can-io-module/lib/diy_dash_can_io.pretty/ECU_FCI_24P_RightAngle.kicad_mod",
    "hardware/can-io-module/bom/can-io-module-bom.csv",
)

REQUIRED_NETS = {
    "VBAT", "POWER_GND", "IGN", "CAN_H", "CAN_L", "K_LINE",
    "SENSOR_5V", "SENSOR_GND", "AIN1", "AIN2", "AIN3", "AIN4",
    "AIN5", "AIN6", "AIN7", "AIN8", "EGT_K_POS", "EGT_K_NEG",
    "RELAY_OUT1", "RELAY_OUT2", "FLEX_IN", "AOUT1", "AOUT2", "SERVICE",
}

REQUIRED_REFS = {
    "U1": "STM32G0B1CBT6",
    "U7": "L9613",
    "U10": "MAX31855KASA+",
}


def validate_project(root: Path) -> list[str]:
    errors: list[str] = []
    for relative in REQUIRED_FILES:
        if not (root / relative).is_file():
            errors.append(f"missing file: {relative}")
    pcb = root / "hardware/can-io-module/can-io-module.kicad_pcb"
    if pcb.is_file():
        text = pcb.read_text(encoding="utf-8")
        for net in sorted(REQUIRED_NETS):
            if f'\"{net}\"' not in text:
                errors.append(f"missing PCB net: {net}")
        if text.count("Edge.Cuts") < 4:
            errors.append("board outline is incomplete")
    bom = root / "hardware/can-io-module/bom/can-io-module-bom.csv"
    if bom.is_file():
        with bom.open(newline="", encoding="utf-8") as handle:
            rows = {row["Reference"]: row for row in csv.DictReader(handle)}
        for ref, value in REQUIRED_REFS.items():
            if rows.get(ref, {}).get("Value") != value:
                errors.append(f"{ref} must be {value}")
    return errors
```

- [ ] **Step 4: Add GitHub-hosted KiCad validation**

Create `.github/workflows/can-io-hardware.yml` that checks out the repository, installs KiCad 9 from `ppa:kicad/kicad-9.0-releases`, runs the Python contract test, runs `kicad-cli sch erc`, runs `kicad-cli pcb drc`, exports schematic PDF and PCB SVG renders, exports Gerbers/drill files into `hardware/can-io-module/manufacturing/generated`, and uploads that directory as the `can-io-hardware-prototype` artifact. Every export step must fail the job on non-zero exit status.

- [ ] **Step 5: Add the prototype warning and project README**

Document that generated fabrication files are mechanical prototypes only until the physical enclosure gate passes. Document exact commands used by CI and the intended 24-pin functions.

- [ ] **Step 6: Run the Python test and confirm expected design-file failures**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL with a list of missing KiCad and BOM files, proving the validator is active.

- [ ] **Step 7: Commit**

```bash
git add test/hardware/test_can_io_hardware.py scripts/check_can_io_hardware.py .github/workflows/can-io-hardware.yml hardware/can-io-module/README.md hardware/can-io-module/manufacturing/prototype-fabrication-notice.txt
git commit -m "test: define CAN IO hardware contract"
```

### Task 2: Connector library and mechanical skeleton

**Files:**
- Create: `hardware/can-io-module/lib/diy_dash_can_io.kicad_sym`
- Create: `hardware/can-io-module/lib/diy_dash_can_io.pretty/ECU_FCI_24P_RightAngle.kicad_mod`
- Create: `hardware/can-io-module/lib/diy_dash_can_io.pretty/ECU_CASE_BOARD_GUIDE.kicad_mod`
- Create: `hardware/can-io-module/sym-lib-table`
- Create: `hardware/can-io-module/fp-lib-table`
- Create: `hardware/can-io-module/can-io-module.kicad_pro`
- Create: `hardware/can-io-module/can-io-module.kicad_pcb`
- Modify: `scripts/check_can_io_hardware.py`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: supplied connector drawings with three rows of eight pins and approximately 3.33 mm horizontal pitch.
- Produces: symbol `DIY_DASH:ECU_FCI_24P`, footprint `DIY_DASH:ECU_FCI_24P_RightAngle`, board guide footprint, closed provisional board outline, and exact pin/net mapping 1–24 from the spec.

- [ ] **Step 1: Extend the failing tests for connector mapping and footprint geometry**

Add assertions that the symbol and footprint each contain pins/pads 1 through 24 exactly once, that the footprint includes fabrication and courtyard outlines, that pin 1 is marked, and that the PCB connector pad-to-net mapping equals the spec table.

- [ ] **Step 2: Run the connector tests to verify they fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL for missing connector libraries and mapping.

- [ ] **Step 3: Create the custom symbol and footprint**

Create a three-bank symbol with power pins grouped separately from sensor and communication pins. Build the right-angle through-hole footprint from the supplied drawing, add pin-1 marking, connector body outline, mating keep-out, mechanical anchor holes, courtyard, and a `MECHANICAL SAMPLE REQUIRED` text on `Cmts.User`.

- [ ] **Step 4: Create the provisional PCB skeleton**

Create a closed two-layer board outline sized to the drawing-derived enclosure cavity, place J1 on the short edge, add enclosure guide/mounting geometry, and reserve zones for POWER, ANALOG, MCU/COMMS, EGT, and OUTPUTS using `Dwgs.User` labels.

- [ ] **Step 5: Run the tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: connector and outline tests PASS; schematic/BOM tests still FAIL.

- [ ] **Step 6: Commit**

```bash
git add hardware/can-io-module/lib hardware/can-io-module/sym-lib-table hardware/can-io-module/fp-lib-table hardware/can-io-module/can-io-module.kicad_pro hardware/can-io-module/can-io-module.kicad_pcb scripts/check_can_io_hardware.py test/hardware/test_can_io_hardware.py
git commit -m "feat: add CAN IO connector and board outline"
```

### Task 3: Power and MCU schematics

**Files:**
- Create: `hardware/can-io-module/can-io-module.kicad_sch`
- Create: `hardware/can-io-module/sheets/power.kicad_sch`
- Create: `hardware/can-io-module/sheets/mcu.kicad_sch`
- Modify: `scripts/check_can_io_hardware.py`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: connector nets `VBAT`, `POWER_GND`, `IGN`, `SENSOR_5V`, `SENSOR_GND`, and the custom J1 symbol.
- Produces: rails `VBAT_PROTECTED`, `+5V`, `+3V3`, `VDDA`, `VREF_MON`, safe IGN logic, MCU nets, SWD pads, reset, clocks, decoupling, and hierarchical labels used by later sheets.

- [ ] **Step 1: Add failing contract checks for the power chain**

Require references and values for input fuse, reverse-polarity MOSFET, `SMBJ33A` TVS, LMR16006-Q1 buck, 3.3 V LDO, SENSOR_5V current limiting, VBAT/5V/3V3 monitor dividers, U1 STM32G0B1CBT6, NRST network, BOOT strap, SWDIO/SWCLK pads, and one decoupling capacitor per MCU supply pin.

- [ ] **Step 2: Run tests to verify the power and MCU checks fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL naming the absent power and MCU references.

- [ ] **Step 3: Draw the root, power, and MCU sheets**

The root sheet contains J1 and five hierarchical sheet symbols. The power sheet implements fused/reverse-protected/surge-filtered VBAT, 5 V buck, 3.3 V LDO, protected SENSOR_5V, IGN conditioning, rail monitoring, and labeled test points. The MCU sheet implements U1, supply filtering, reset/boot, SWD, timer/DAC/ADC assignments, and safe default pull resistors.

- [ ] **Step 4: Run tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: power and MCU contract checks PASS.

- [ ] **Step 5: Commit**

```bash
git add hardware/can-io-module/can-io-module.kicad_sch hardware/can-io-module/sheets/power.kicad_sch hardware/can-io-module/sheets/mcu.kicad_sch scripts/check_can_io_hardware.py test/hardware/test_can_io_hardware.py
git commit -m "feat: add CAN IO power and MCU schematics"
```

### Task 4: Eight universal analog-input channels

**Files:**
- Create: `hardware/can-io-module/sheets/analog-inputs.kicad_sch`
- Modify: `scripts/check_can_io_hardware.py`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: `AIN1..AIN8`, `+5V`, `+3V3`, `SENSOR_GND`, MCU ADC nets `ADC_IN1..ADC_IN8`, and mux controls `PULL_A0..PULL_A2`, `PULL_1K_EN_N`, `PULL_4K7_EN_N`.
- Produces: eight protected divided ADC signals and independently time-selected 1 kOhm or 4.7 kOhm excitation.

- [ ] **Step 1: Add failing channel-symmetry and safety tests**

For every channel 1–8 require connector-side ESD, 9.98–10.02 kOhm series resistance, 15 kOhm lower divider, 100 nF ADC filter, clamp device, ADC label, and connection to both excitation multiplexers. Require 100 kOhm pull-ups on both mux enable pins so both devices remain disabled at reset. Calculate and assert `5.0 * 15000 / (10000 + 15000) <= 3.1`.

- [ ] **Step 2: Run the test to verify all eight channels fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL listing AIN1 through AIN8 and both mux safe-state checks.

- [ ] **Step 3: Draw the complete analog-input sheet**

Use eight repeated protected divider/filter blocks. Add U5 and U6 as 74HCT4051 devices powered from 5 V, connect their common pins through 1.00 kOhm and 4.70 kOhm 1% resistors to 5 V, respectively, and connect each selected branch to the connector side of its input network. Add explicit net labels and test points at AIN and ADC sides.

- [ ] **Step 4: Run tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: all analog-input tests PASS.

- [ ] **Step 5: Commit**

```bash
git add hardware/can-io-module/sheets/analog-inputs.kicad_sch scripts/check_can_io_hardware.py test/hardware/test_can_io_hardware.py
git commit -m "feat: add configurable universal analog inputs"
```

### Task 5: CAN, fixed L9613 K-line, EGT, and Flex Fuel

**Files:**
- Create: `hardware/can-io-module/sheets/communications-egt.kicad_sch`
- Modify: `scripts/check_can_io_hardware.py`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: `CAN_H`, `CAN_L`, `K_LINE`, `EGT_K_POS`, `EGT_K_NEG`, `FLEX_IN`, `+5V`, `+3V3`, `VBAT_PROTECTED`, and MCU communication nets.
- Produces: protected MCU-level FDCAN, UART K-line, SPI EGT, and timer-capture Flex Fuel signals.

- [ ] **Step 1: Add failing checks for all communication paths**

Require TJA1051T/3, CAN TVS, common-mode choke bypass option, open-by-default 120 Ohm termination; L9613 only with 3.3/5 V TX/RX level conditioning, K-line TVS, and hardware off/high-impedance control; MAX31855KASA+ with symmetric EGT RC filtering and no vias in the schematic-defined EGT net class; and a protected Schmitt Flex Fuel input with 5 V pull-up.

- [ ] **Step 2: Run tests to verify communication checks fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL for CAN, L9613, EGT, and Flex references/nets.

- [ ] **Step 3: Draw the communications and EGT sheet**

Use L9613 as the only K-line transceiver. Power its bus side from protected vehicle supply and its logic interface through explicit level conditioning so STM32 pins never see 5 V. Include a hardware-controlled disable/high-impedance path. Place the EGT connector labels directly beside MAX31855 input filtering and label both paths as matched. Add test points for CAN TX/RX, KTX/KRX, EGT SPI, and FLEX_CAPTURE.

- [ ] **Step 4: Run tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: communication/EGT checks PASS.

- [ ] **Step 5: Commit**

```bash
git add hardware/can-io-module/sheets/communications-egt.kicad_sch scripts/check_can_io_hardware.py test/hardware/test_can_io_hardware.py
git commit -m "feat: add CAN K-line EGT and Flex interfaces"
```

### Task 6: Analog and relay outputs

**Files:**
- Create: `hardware/can-io-module/sheets/outputs.kicad_sch`
- Modify: `scripts/check_can_io_hardware.py`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: DAC nets `DAC1_RAW`, `DAC2_RAW`, relay-control nets, `+5V`, `+3V3`, `VBAT_PROTECTED`, and `POWER_GND`.
- Produces: protected `AOUT1`, `AOUT2`, `RELAY_OUT1`, and `RELAY_OUT2` with safe reset defaults.

- [ ] **Step 1: Add failing output safety tests**

Require TLV9002-Q1, two gain/feedback networks with calibration headroom, two 220 Ohm isolation resistors, output RC filters and ESD devices, two 60 V logic-level MOSFET relay drivers, gate pull-downs, flyback suppression, and OFF defaults. Check that the BOM and README specify AOUT load impedance of at least 10 kOhm and relay current no greater than 500 mA.

- [ ] **Step 2: Run tests to verify output checks fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL for missing output components and safe-state networks.

- [ ] **Step 3: Draw the outputs sheet**

Use TLV9002-Q1 on 5 V with each DAC in a non-inverting stage sized to reach calibrated 5.0 V before maximum DAC code. Add connector-side output protection. Use two identical low-side relay stages with 100 kOhm gate pull-downs and external-coil flyback paths to protected VBAT.

- [ ] **Step 4: Run tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: output safety tests PASS.

- [ ] **Step 5: Commit**

```bash
git add hardware/can-io-module/sheets/outputs.kicad_sch scripts/check_can_io_hardware.py test/hardware/test_can_io_hardware.py
git commit -m "feat: add CAN IO analog and relay outputs"
```

### Task 7: BOM and approved substitutions

**Files:**
- Create: `hardware/can-io-module/bom/can-io-module-bom.csv`
- Create: `hardware/can-io-module/bom/approved-alternates.csv`
- Modify: `scripts/check_can_io_hardware.py`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: every schematic reference and value.
- Produces: buildable BOM with columns `Reference,Quantity,Value,Manufacturer,MPN,Package,Populate,Notes` and alternates with electrical constraints rather than unverified drop-ins.

- [ ] **Step 1: Add failing BOM completeness tests**

Require every populated schematic reference in the BOM exactly once, reject duplicate references, require manufacturer part number and package for every IC/protection device, require L9613 and reject L9637, and require `DNP` entries for selectable CAN termination and configuration links.

- [ ] **Step 2: Run tests to verify BOM checks fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL because BOM files are absent.

- [ ] **Step 3: Create BOM and alternates list**

Use actual selected orderable parts. Alternates must state minimum voltage, temperature, bandwidth, leakage, package, and pin-compatibility requirements. Do not list an alternate if those properties are unverified.

- [ ] **Step 4: Run tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: BOM tests PASS and the overall Python contract reaches PASS before routing-specific checks are added.

- [ ] **Step 5: Commit**

```bash
git add hardware/can-io-module/bom scripts/check_can_io_hardware.py test/hardware/test_can_io_hardware.py
git commit -m "docs: add CAN IO module BOM"
```

### Task 8: Component placement, routing, and copper

**Files:**
- Modify: `hardware/can-io-module/can-io-module.kicad_pcb`
- Modify: `scripts/check_can_io_hardware.py`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: complete annotated schematic and footprint assignments.
- Produces: routed two-layer PCB with defined net classes, ground pours, keep-outs, test points, silkscreen labels, and no unrouted connections.

- [ ] **Step 1: Add failing layout contract tests**

Require zero unconnected pads in the PCB connectivity data, explicit net classes for POWER, CAN, EGT, ANALOG, and RELAY, 0.5 mm or wider VBAT/5V/relay traces, a continuous bottom GND zone, CAN differential pair constraints, EGT keep-out from power/output zones, and placement-zone membership for all major references.

- [ ] **Step 2: Run tests to verify routing checks fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL listing unrouted nets and missing net classes/zones.

- [ ] **Step 3: Place components by functional zone**

Place connector protection first, then power near pins 1–3, CAN/K-line near pins 4–6, analog protection/muxes near pins 9–16, MAX31855 immediately behind pins 17–18, output protection near pins 19–23, and MCU centrally. Keep the buck switch node and relay drains at the opposite side from EGT and ADC filters.

- [ ] **Step 4: Route critical networks**

Route EGT first without vias where possible, CAN second as a coupled pair, power/relay paths third, ADC/pull-up networks fourth, and remaining digital signals last. Fill the bottom GND zone and add top copper only where it does not introduce thermal asymmetry near EGT.

- [ ] **Step 5: Add manufacturing and service markings**

Add pin numbers and abbreviated functions, board name/revision, polarity, CAN termination state, SWD labels, test-point labels, prototype warning, and a clear pin-1 indicator.

- [ ] **Step 6: Run the Python layout tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add hardware/can-io-module/can-io-module.kicad_pcb scripts/check_can_io_hardware.py test/hardware/test_can_io_hardware.py
git commit -m "feat: route CAN IO module PCB"
```

### Task 9: Bring-up documentation and GitHub fabrication validation

**Files:**
- Create: `hardware/can-io-module/bringup-checklist.md`
- Create: `hardware/can-io-module/manufacturing/README.md`
- Create: `scripts/export_can_io_hardware.ps1`
- Modify: `.github/workflows/can-io-hardware.yml`
- Modify: `hardware/can-io-module/README.md`
- Modify: `test/hardware/test_can_io_hardware.py`

**Interfaces:**
- Consumes: completed KiCad project and BOM.
- Produces: reproducible GitHub artifact containing ERC/DRC reports, schematic PDF, PCB renders, position files, Gerbers, drills, and BOM, plus a stepwise safe bring-up procedure.

- [ ] **Step 1: Add failing documentation/export tests**

Require the bring-up checklist to cover current-limited first power, rail measurements, SWD access, reset pull-up safety, CAN silent test, K-line high-impedance test, analog calibration, EGT simulator test, output safe-state test, relay dummy-load test, and enclosure fit gate. Require the workflow to upload ERC and DRC reports even when a later export fails.

- [ ] **Step 2: Run tests to verify documentation checks fail**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: FAIL for absent bring-up/export documentation.

- [ ] **Step 3: Write bring-up and manufacturing instructions**

Document exact test equipment, current limits, expected rail ranges, pass/fail conditions, pinout, warnings, and the fact that generated Gerbers remain prototype-only until physical mechanical validation.

- [ ] **Step 4: Implement reproducible export commands**

The PowerShell script mirrors CI commands for developers with KiCad 9 installed, but is not executed in this environment. It creates a clean output directory and runs KiCad CLI ERC, DRC, PDF/SVG plotting, Gerber, drill, position, and BOM exports without modifying source files.

- [ ] **Step 5: Run all non-KiCad contract tests**

Run: `python -m unittest test.hardware.test_can_io_hardware -v`

Expected: PASS.

- [ ] **Step 6: Push the branch and let GitHub run KiCad validation**

Run: `git push origin feature/racechrono-ble-monitor`

Expected: GitHub workflow `can-io-hardware` completes Python contract, ERC, DRC, renders, and prototype fabrication archive.

- [ ] **Step 7: Inspect GitHub reports and fix every unexplained issue**

Review uploaded ERC and DRC reports. Any waived issue must be converted into an explicit, documented KiCad exclusion with a reason; do not suppress errors only in CI.

- [ ] **Step 8: Commit final validation/documentation fixes**

```bash
git add hardware/can-io-module .github/workflows/can-io-hardware.yml scripts/export_can_io_hardware.ps1 test/hardware/test_can_io_hardware.py
git commit -m "docs: finalize CAN IO hardware package"
```

## Self-review result

- Every hardware requirement in the approved spec maps to Tasks 2–9.
- Firmware and dashboard CAN configuration implementation are intentionally excluded from this hardware plan and will receive a separate spec/plan after the PCB interface is stable.
- L9613 is the only K-line transceiver in the BOM and tests explicitly reject L9637.
- The five high-risk review-focus cases each have an owning test in Tasks 3–8.
- No production approval is implied before the physical enclosure/connector fit gate.

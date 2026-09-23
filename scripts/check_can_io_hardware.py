from __future__ import annotations

import csv
from pathlib import Path
import re


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

CONNECTOR_PINOUT = {
    1: "VBAT", 2: "POWER_GND", 3: "IGN", 4: "CAN_H", 5: "CAN_L", 6: "K_LINE",
    7: "SENSOR_5V", 8: "SENSOR_GND", 9: "AIN1", 10: "AIN2", 11: "AIN3",
    12: "AIN4", 13: "AIN5", 14: "AIN6", 15: "AIN7", 16: "AIN8",
    17: "EGT_K_POS", 18: "EGT_K_NEG", 19: "RELAY_OUT1", 20: "RELAY_OUT2",
    21: "FLEX_IN", 22: "AOUT1", 23: "AOUT2", 24: "SERVICE",
}


def validate_connector(root: Path) -> list[str]:
    errors: list[str] = []
    base = root / "hardware/can-io-module"
    symbol = base / "lib/diy_dash_can_io.kicad_sym"
    footprint = base / "lib/diy_dash_can_io.pretty/ECU_FCI_24P_RightAngle.kicad_mod"
    pcb = base / "can-io-module.kicad_pcb"

    if not symbol.is_file():
        errors.append("missing connector symbol")
    else:
        text = symbol.read_text(encoding="utf-8")
        pins = [int(value) for value in re.findall(r'\(number "(\d+)"', text)]
        if sorted(pins) != list(range(1, 25)):
            errors.append("connector symbol must contain pins 1..24 exactly once")

    if not footprint.is_file():
        errors.append("missing connector footprint")
    else:
        text = footprint.read_text(encoding="utf-8")
        pads = [int(value) for value in re.findall(r'\(pad "(\d+)"', text)]
        if sorted(pads) != list(range(1, 25)):
            errors.append("connector footprint must contain pads 1..24 exactly once")
        for marker in ("F.Fab", "F.CrtYd", "PIN 1", "MECHANICAL SAMPLE REQUIRED"):
            if marker not in text:
                errors.append(f"connector footprint missing {marker}")

    if not pcb.is_file():
        errors.append("missing PCB connector mapping")
    else:
        text = pcb.read_text(encoding="utf-8")
        for pin, net in CONNECTOR_PINOUT.items():
            pattern = rf'\(pad "{pin}"[^\n]*\(net \d+ "{re.escape(net)}"\)'
            if not re.search(pattern, text):
                errors.append(f"J1 pin {pin} must map to {net}")
    return errors


def _require_tokens(path: Path, tokens: tuple[str, ...], scope: str) -> list[str]:
    if not path.is_file():
        return [f"missing {scope}: {path.name}"]
    text = path.read_text(encoding="utf-8")
    return [f"{scope} missing {token}" for token in tokens if token not in text]


def validate_power_and_mcu(root: Path) -> list[str]:
    sheets = root / "hardware/can-io-module/sheets"
    power_tokens = (
        "F1 1A INPUT FUSE", "Q1 REVERSE POLARITY 60V", "D1 SMBJ33A",
        "U2 LMR16006-Q1", "U3 3V3 LDO 300mA", "SENSOR_5V 250mA CURRENT LIMIT",
        "VBAT_MON", "VREF_MON", "3V3_MON", "IGN_SCHMITT", "VBAT_PROTECTED",
    )
    mcu_tokens = (
        "U1 STM32G0B1CBT6", "NRST 10k PULLUP", "BOOT0 100k PULLDOWN",
        "SWDIO", "SWCLK", "TP_NRST", "VDDA FILTER", "C_VDD1 100nF",
        "C_VDD2 100nF", "C_VDDA 100nF", "DAC1_RAW", "DAC2_RAW",
    )
    return (
        _require_tokens(sheets / "power.kicad_sch", power_tokens, "power sheet")
        + _require_tokens(sheets / "mcu.kicad_sch", mcu_tokens, "MCU sheet")
    )


def validate_project(root: Path) -> list[str]:
    errors: list[str] = []
    for relative in REQUIRED_FILES:
        if not (root / relative).is_file():
            errors.append(f"missing file: {relative}")
    errors.extend(validate_connector(root))
    errors.extend(validate_power_and_mcu(root))

    pcb = root / "hardware/can-io-module/can-io-module.kicad_pcb"
    if pcb.is_file():
        text = pcb.read_text(encoding="utf-8")
        for net in sorted(REQUIRED_NETS):
            if f'"{net}"' not in text:
                errors.append(f"missing PCB net: {net}")
        if '(gr_rect' not in text or '(layer "Edge.Cuts")' not in text:
            errors.append("board outline is incomplete")

    bom = root / "hardware/can-io-module/bom/can-io-module-bom.csv"
    if bom.is_file():
        with bom.open(newline="", encoding="utf-8") as handle:
            rows = {row["Reference"]: row for row in csv.DictReader(handle)}
        for ref, value in REQUIRED_REFS.items():
            if rows.get(ref, {}).get("Value") != value:
                errors.append(f"{ref} must be {value}")
    return errors


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[1]
    findings = validate_project(repository)
    if findings:
        print("\n".join(findings))
        raise SystemExit(1)
    print("CAN I/O hardware contract: PASS")

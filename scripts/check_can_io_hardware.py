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
            if f'"{net}"' not in text:
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


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[1]
    findings = validate_project(repository)
    if findings:
        print("\n".join(findings))
        raise SystemExit(1)
    print("CAN I/O hardware contract: PASS")

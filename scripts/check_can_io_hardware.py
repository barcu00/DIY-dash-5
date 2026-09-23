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


def validate_analog_inputs(root: Path) -> list[str]:
    path = root / "hardware/can-io-module/sheets/analog-inputs.kicad_sch"
    tokens = [
        "U5 74HCT4051 1K EXCITATION MUX", "U6 74HCT4051 4K7 EXCITATION MUX",
        "PULL_1K_EN_N 100k PULLUP TO +5V", "PULL_4K7_EN_N 100k PULLUP TO +5V",
        "5.0 * 15000 / (10000 + 15000) = 3.000V",
    ]
    for channel in range(1, 9):
        tokens.extend((
            f"AIN{channel} CONNECTOR ESD", f"AIN{channel} SERIES 10.0k 1%",
            f"AIN{channel} LOWER 15.0k 1%", f"ADC_IN{channel} FILTER 100nF",
            f"ADC_IN{channel} LOW-LEAKAGE CLAMP", f"AIN{channel} -> U5.Y{channel - 1}",
            f"AIN{channel} -> U6.Y{channel - 1}",
        ))
    errors = _require_tokens(path, tuple(tokens), "analog input sheet")
    if 5.0 * 15000 / (10000 + 15000) > 3.1:
        errors.append("5V analog divider exceeds 3.1V ADC limit")
    return errors


def validate_communications(root: Path) -> list[str]:
    path = root / "hardware/can-io-module/sheets/communications-egt.kicad_sch"
    tokens = (
        "U8 TJA1051T/3", "CAN TVS", "CAN COMMON-MODE CHOKE BYPASS",
        "JP1 120R TERMINATION DEFAULT OPEN", "U7 L9613", "L9637 FORBIDDEN",
        "K-LINE TVS", "KTX 3V3-TO-5V LEVEL SHIFT", "KRX 5V-TO-3V3 LEVEL SHIFT",
        "KLINE_OFF HIGH IMPEDANCE", "U10 MAX31855KASA+", "EGT_K_POS MATCHED RC",
        "EGT_K_NEG MATCHED RC", "EGT NO VIAS", "U11 SCHMITT FLEX FRONT END",
        "FLEX_IN 5V PULLUP", "FLEX_CAPTURE", "TP_CAN_TX", "TP_CAN_RX",
        "TP_KTX", "TP_KRX", "TP_EGT_SCK", "TP_FLEX_CAPTURE",
    )
    errors = _require_tokens(path, tokens, "communications/EGT sheet")
    if path.is_file() and "L9637" in path.read_text(encoding="utf-8").replace("L9637 FORBIDDEN", ""):
        errors.append("L9637 must not be used; L9613 is fixed")
    return errors


def validate_outputs(root: Path) -> list[str]:
    path = root / "hardware/can-io-module/sheets/outputs.kicad_sch"
    tokens = (
        "U9 TLV9002-Q1", "AOUT1 NON-INVERTING GAIN 1.55", "AOUT2 NON-INVERTING GAIN 1.55",
        "AOUT1 220R ISOLATION", "AOUT2 220R ISOLATION", "AOUT1 RC FILTER + ESD",
        "AOUT2 RC FILTER + ESD", "AOUT LOAD >= 10k", "AOUT STARTUP 0V",
        "Q4 60V LOGIC N-MOS RELAY1", "Q5 60V LOGIC N-MOS RELAY2",
        "RELAY1 GATE 100k PULLDOWN", "RELAY2 GATE 100k PULLDOWN",
        "RELAY1 FLYBACK TO VBAT_PROTECTED", "RELAY2 FLYBACK TO VBAT_PROTECTED",
        "RELAY CURRENT <= 500mA", "OUTPUTS OFF DURING RESET",
    )
    return _require_tokens(path, tokens, "outputs sheet")


def validate_bom(root: Path) -> list[str]:
    bom = root / "hardware/can-io-module/bom/can-io-module-bom.csv"
    alternates = root / "hardware/can-io-module/bom/approved-alternates.csv"
    errors: list[str] = []
    if not bom.is_file():
        errors.append("missing CAN IO BOM")
        return errors
    required_columns = ("Reference", "Quantity", "Value", "Manufacturer", "MPN", "Package", "Populate", "Notes")
    with bom.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        if tuple(reader.fieldnames or ()) != required_columns:
            errors.append("BOM columns do not match contract")
        rows = list(reader)
    refs = [row["Reference"] for row in rows]
    if len(refs) != len(set(refs)):
        errors.append("BOM contains duplicate references")
    for required in ("U1", "U2", "U3", "U5", "U6", "U7", "U8", "U9", "U10", "U11", "D1", "JP1"):
        if required not in refs:
            errors.append(f"BOM missing {required}")
    values = " ".join(row["Value"] for row in rows)
    if "L9613" not in values or "L9637" in values:
        errors.append("BOM must contain L9613 and reject L9637")
    for row in rows:
        if row["Reference"].startswith(("U", "D", "Q", "TVS")) and (not row["MPN"] or not row["Package"]):
            errors.append(f"{row['Reference']} lacks MPN or package")
    jp1 = next((row for row in rows if row["Reference"] == "JP1"), {})
    if jp1.get("Populate") != "DNP":
        errors.append("JP1 CAN termination must default DNP")
    if not alternates.is_file():
        errors.append("missing approved alternates table")
    else:
        text = alternates.read_text(encoding="utf-8")
        for token in ("MinimumVoltage", "TemperatureRange", "Bandwidth", "MaxLeakage", "Package", "PinCompatible"):
            if token not in text:
                errors.append(f"alternates missing {token}")
    return errors


def validate_layout(root: Path) -> list[str]:
    pcb = root / "hardware/can-io-module/can-io-module.kicad_pcb"
    tokens = (
        "NETCLASS POWER WIDTH 0.60MM", "NETCLASS CAN DIFF 0.25MM GAP 0.20MM",
        "NETCLASS EGT WIDTH 0.20MM NO VIAS", "NETCLASS ANALOG WIDTH 0.20MM",
        "NETCLASS RELAY WIDTH 0.60MM", "BOTTOM POWER_GND PLANE",
        "EGT COPPER KEEPOUT", "BUCK SWITCH NODE KEEPOUT", "PROTOTYPE REV A",
        "PIN 1 VBAT", "CAN TERM DEFAULT OPEN", "SWDIO", "SWCLK",
        "ZONE_POWER", "ZONE_MCU_COMMS", "ZONE_ANALOG", "ZONE_EGT", "ZONE_OUTPUTS",
    )
    errors = _require_tokens(pcb, tokens, "PCB layout")
    if pcb.is_file():
        text = pcb.read_text(encoding="utf-8")
        for ref in ("J1", "U1", "U2", "U5", "U6", "U7", "U8", "U9", "U10", "U11", "Q4", "Q5"):
            if f'"{ref}"' not in text:
                errors.append(f"PCB placement missing {ref}")
        if '(zone (net 2) (net_name "POWER_GND") (layer "B.Cu")' not in text:
            errors.append("PCB missing continuous bottom POWER_GND zone")
    return errors


def validate_documentation(root: Path) -> list[str]:
    checklist = root / "hardware/can-io-module/bringup-checklist.md"
    manufacturing = root / "hardware/can-io-module/manufacturing/README.md"
    export = root / "scripts/export_can_io_hardware.ps1"
    workflow = root / ".github/workflows/can-io-hardware.yml"
    tokens = (
        "current-limited first power", "5 V rail", "3.3 V rail", "SWD access",
        "mux reset safety", "CAN silent", "K-line high-impedance",
        "analog calibration", "thermocouple simulator", "output safe state",
        "relay dummy load", "physical enclosure fit gate",
    )
    errors = _require_tokens(checklist, tokens, "bring-up checklist")
    errors += _require_tokens(manufacturing, ("PROTOTYPE ONLY", "1:1", "Gerber", "drill", "position"), "manufacturing guide")
    errors += _require_tokens(export, ("sch erc", "pcb drc", "sch export pdf", "pcb export svg", "pcb export gerbers", "pcb export drill", "pcb export pos"), "export script")
    errors += _require_tokens(workflow, ("if: always()", "erc-report.rpt", "drc-report.rpt", "upload-artifact@v4"), "hardware workflow")
    return errors


def validate_project(root: Path) -> list[str]:
    errors: list[str] = []
    for relative in REQUIRED_FILES:
        if not (root / relative).is_file():
            errors.append(f"missing file: {relative}")
    errors.extend(validate_connector(root))
    errors.extend(validate_power_and_mcu(root))
    errors.extend(validate_analog_inputs(root))
    errors.extend(validate_communications(root))
    errors.extend(validate_outputs(root))
    errors.extend(validate_bom(root))
    errors.extend(validate_layout(root))
    errors.extend(validate_documentation(root))

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

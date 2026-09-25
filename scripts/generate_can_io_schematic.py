"""Generate the electrically connected KiCad 9 CAN I/O schematic.

The generator intentionally uses pin-numbered passive connector bodies for the
critical ICs.  That keeps the checked-in netlist unambiguous: each label is
attached to the exact manufacturer package pin documented in
``electrical-pin-map.json``.  The value, footprint and reference retain the
real production part identity.
"""
from __future__ import annotations

import csv
from pathlib import Path

from kicad_sch_api import create_schematic

from generate_can_io_routed_pcb import CONNECTOR, mappings


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "hardware/can-io-module"
BOM = BASE / "bom/can-io-module-bom.csv"
OUTPUT = BASE / "can-io-module.kicad_sch"


def symbol_for_pin_count(count: int) -> str:
    if not 1 <= count <= 48:
        raise ValueError(f"unsupported schematic pin count: {count}")
    return f"Connector_Generic:Conn_01x{count:02d}"


def footprint_for(reference: str, package: str) -> str:
    if reference == "J1":
        return "DIY_DASH:ECU_FCI_24P_RightAngle"
    if reference == "J2":
        return "DIY_DASH:SWD_1x05_P2.54mm_Vertical"
    return f'DIY_DASH:{package.replace(" ", "_")}'


def build_components() -> list[dict[str, object]]:
    rows = list(csv.DictReader(BOM.open(encoding="utf-8", newline="")))
    pin_maps = mappings()
    components: list[dict[str, object]] = [
        {
            "Reference": "J1",
            "Value": "ECU_FCI_24P_RightAngle",
            "Package": "Custom THT 3x8",
            "Pins": CONNECTOR,
        },
        {
            "Reference": "J2",
            "Value": "SWD_1x05_P2.54mm_DNP",
            "Package": "PinHeader_1x05_P2.54mm_Vertical",
            "Pins": ["+3V3", "SWDIO", "SWCLK", "NRST", "POWER_GND"],
        },
    ]
    for row in rows:
        reference = row["Reference"]
        if reference in {"J1", "J2"}:
            continue
        pins = pin_maps.get(reference)
        if pins is None:
            pins = [None] if reference.startswith("TP") else [None, None]
        components.append({**row, "Pins": pins})
    return components


def main() -> None:
    schematic = create_schematic("CAN I/O Module - electrical source of truth")
    schematic._data["paper"] = "A1"
    schematic._data["title_block"] = {
        "title": "DIY Dash 5 CAN I/O Module",
        "date": "2026-09-25",
        "rev": "B-ELECTRICAL",
        "company": "DIY Dash 5",
        "comment": {
            "1": "PROTOTYPE ONLY - physical connector and enclosure fit gate required",
            "2": "Two-layer design; GitHub ERC/DRC is the automated release gate",
        },
    }
    schematic.add_text(
        "CAN I/O MODULE - ELECTRICAL SOURCE OF TRUTH",
        (35, 20),
        size=2.5,
        bold=True,
    )
    schematic.add_text(
        "Every net label is wired to the numbered physical package pin. J2: 1 +3V3, 2 SWDIO, 3 SWCLK, 4 NRST, 5 POWER_GND.",
        (35, 26),
        size=1.1,
    )

    components = build_components()
    columns = [70, 215, 360, 505, 650]
    next_y = [45.0] * len(columns)
    for index, item in enumerate(components):
        reference = str(item["Reference"])
        value = str(item["Value"])
        package = str(item["Package"])
        pins = list(item["Pins"])
        column = min(range(len(columns)), key=lambda candidate: next_y[candidate])
        x = columns[column]
        y = next_y[column] + max(8.0, len(pins) * 1.27)
        component = schematic.components.add(
            symbol_for_pin_count(len(pins)),
            reference,
            value,
            (x, y),
            footprint=footprint_for(reference, package),
        )
        for pin_number, net in enumerate(pins, 1):
            pin = str(pin_number)
            position = component.get_pin_position(pin)
            if position is None:
                raise RuntimeError(f"cannot locate {reference} pin {pin}")
            if net:
                label_position = (position.x - 2.54, position.y)
                schematic.add_wire(position, label_position)
                schematic.add_label(str(net), position=label_position, rotation=0, size=0.9)
            else:
                schematic.no_connects.add(position)
        schematic.add_text(
            f"{reference} / {package}",
            (x - 18, y - max(7.0, len(pins) * 1.27) - 2.5),
            size=0.8,
            bold=True,
        )
        next_y[column] = y + max(15.0, len(pins) * 1.27 + 10.0)

    schematic.save(OUTPUT, preserve_format=False)
    print(
        f"generated {OUTPUT}: {len(components)} components, "
        f"{sum(len(list(item['Pins'])) for item in components)} physical pins"
    )


if __name__ == "__main__":
    main()

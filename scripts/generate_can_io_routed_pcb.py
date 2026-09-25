"""Generate the CAN I/O Revision-B electrical-routing prototype.

The generator is intentionally deterministic: component positions, pad/net assignment,
and preliminary routing remain reviewable in git. KiCad ERC/DRC is the release gate.
"""
from __future__ import annotations

import csv
from pathlib import Path
import uuid

ROOT = Path(__file__).resolve().parents[1]
BOARD = ROOT / "hardware/can-io-module/can-io-module.kicad_pcb"
BOM = ROOT / "hardware/can-io-module/bom/can-io-module-bom.csv"

CONNECTOR = [
    "VBAT", "POWER_GND", "IGN", "CAN_H", "CAN_L", "K_LINE", "SENSOR_5V", "SENSOR_GND",
    "AIN1", "AIN2", "AIN3", "AIN4", "AIN5", "AIN6", "AIN7", "AIN8",
    "EGT_K_POS", "EGT_K_NEG", "RELAY_OUT1", "RELAY_OUT2", "FLEX_IN", "AOUT1", "AOUT2", "SERVICE",
]

NETS = CONNECTOR + [
    "+5V", "+3V3", "VBAT_FUSED", "VBAT_PROTECTED", "BUCK_SW", "BUCK_FB", "BUCK_BOOT",
    "NRST", "BOOT0", "VDDA", "SWDIO", "SWCLK", "CAN_TX", "CAN_RX", "CAN_SILENT",
    "KTX", "KRX", "EGT_SCK", "EGT_CS", "EGT_SO", "FLEX_CAPTURE", "DAC1_RAW", "DAC2_RAW",
    "RELAY1_GATE", "RELAY2_GATE", "PULL_A0", "PULL_A1", "PULL_A2", "PULL_1K_EN_N",
    "PULL_4K7_EN_N", "EXC_1K", "EXC_4K7", "AOUT1_OP", "AOUT2_OP",
    "VBAT_MON", "V5_MON", "V3V3_MON", "SENSOR_5V_EN", "SENSOR_5V_OC", "SENSOR_5V_SW",
    "IGN_SENSE", "KLINE_ENABLE", "CAN_H_PROTECTED", "CAN_L_PROTECTED",
] + [f"ADC_IN{i}" for i in range(1, 9)]
NET_ID = {name: index + 1 for index, name in enumerate(NETS)}


def uid(tag: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "diy-dash-can-io/" + tag))


def net_clause(name: str | None) -> str:
    return "" if not name else f' (net {NET_ID[name]} "{name}")'


def footprint(ref: str, value: str, x: float, y: float, pads: list[str | None], package: str) -> tuple[str, list[tuple[str, float, float]]]:
    count = len(pads)
    coords: list[tuple[float, float]] = []
    sizes: list[tuple[float, float]] = []
    if count == 1:
        coords = [(0, 0)]
    elif count == 2:
        coords = [(-1.0, 0), (1.0, 0)]
    elif count <= 6:
        left = (count + 1) // 2
        coords = [(-1.4, (i - (left - 1) / 2) * 1.0) for i in range(left)]
        coords += [(1.4, ((count - left - 1) / 2 - i) * 1.0) for i in range(count - left)]
    elif count <= 16:
        half = count // 2
        coords = [(-2.7, (i - (half - 1) / 2) * 1.27) for i in range(half)]
        coords += [(2.7, ((count - half - 1) / 2 - i) * 1.27) for i in range(count - half)]
    else:
        side = count // 4
        pitch = 0.5
        for i in range(side): coords.append((-4.4, (i - (side - 1) / 2) * pitch))
        for i in range(side): coords.append(((i - (side - 1) / 2) * pitch, 4.4))
        for i in range(side): coords.append((4.4, ((side - 1) / 2 - i) * pitch))
        for i in range(count - 3 * side): coords.append((((side - 1) / 2 - i) * pitch, -4.4))
        sizes = [(1.20, 0.25)] * side + [(0.25, 1.20)] * side
        sizes += [(1.20, 0.25)] * side + [(0.25, 1.20)] * (count - 3 * side)
    if not sizes:
        sizes = [(1.15, 0.75)] * count
    body_x = 5.8 if count <= 16 else 9.0
    body_y = max(3.2, (max((abs(c[1]) for c in coords), default=0) * 2 + 2)) if count <= 16 else 9.0
    lines = [
        f'  (footprint "DIY_DASH:{package.replace(" ", "_")}" (layer "F.Cu") (tstamp {uid(ref)})',
        f'    (at {x:.2f} {y:.2f})',
        f'    (property "Reference" "{ref}" (at 0 {-body_y/2-1.0:.2f}) (layer "F.SilkS") (effects (font (size 0.75 0.75) (thickness 0.12))))',
        f'    (property "Value" "{value}" (at 0 {body_y/2+1.0:.2f}) (layer "F.Fab") (effects (font (size 0.6 0.6) (thickness 0.10))))',
        f'    (fp_rect (start {-body_x/2:.2f} {-body_y/2:.2f}) (end {body_x/2:.2f} {body_y/2:.2f}) (stroke (width 0.15) (type default)) (fill none) (layer "F.SilkS") (tstamp {uid(ref+"-body")}))',
    ]
    connected: list[tuple[str, float, float]] = []
    for index, ((dx, dy), (sx, sy), net) in enumerate(zip(coords, sizes, pads), 1):
        shape = "roundrect" if index == 1 else "rect"
        lines.append(f'    (pad "{index}" smd {shape} (at {dx:.2f} {dy:.2f}) (size {sx:.2f} {sy:.2f}) (layers "F.Cu" "F.Paste" "F.Mask"){net_clause(net)} (roundrect_rratio 0.2) (tstamp {uid(ref+"-"+str(index))}))')
        if net: connected.append((net, x + dx, y + dy))
    lines.append("  )")
    return "\n".join(lines), connected


def j1() -> tuple[str, list[tuple[str, float, float]]]:
    x, y = 56.0, 25.0
    lines = [
        f'  (footprint "DIY_DASH:ECU_FCI_24P_RightAngle" (layer "F.Cu") (tstamp {uid("J1")})',
        f'    (at {x} {y})',
        '    (property "Reference" "J1" (at 11.65 -5) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))',
        '    (property "Value" "ECU_FCI_24P_RightAngle" (at 11.65 13.5) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))',
        f'    (fp_rect (start -3.8 -3) (end 27.11 11.3) (stroke (width 0.25) (type default)) (fill none) (layer "F.SilkS") (tstamp {uid("J1-body")}))',
        f'    (fp_text user "PIN 1" (at -2 -1.5) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.15))) (tstamp {uid("J1-pin1")}))',
    ]
    connected = []
    for i, net in enumerate(CONNECTOR):
        dx, dy = (i % 8) * 3.33, (i // 8) * 4.15
        shape = "rect" if i == 0 else "oval"
        lines.append(f'    (pad "{i+1}" thru_hole {shape} (at {dx:.2f} {dy:.2f}) (size 2.3 2.3) (drill 1.2) (layers "*.Cu" "*.Mask"){net_clause(net)} (pinfunction "{net}") (pintype "passive") (tstamp {uid("J1-"+str(i+1))}))')
        connected.append((net, x + dx, y + dy))
    lines.append("  )")
    return "\n".join(lines), connected


def j2() -> tuple[str, list[tuple[str, float, float]]]:
    """Unpopulated standard-pitch SWD service header."""
    x, y = 86.0, 92.0
    nets = ["+3V3", "SWDIO", "SWCLK", "NRST", "POWER_GND"]
    lines = [
        f'  (footprint "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" (layer "F.Cu") (tstamp {uid("J2")})',
        f'    (at {x} {y})',
        '    (property "Reference" "J2" (at 0 -2.33) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))',
        '    (property "Value" "SWD_1x05_P2.54mm_DNP" (at 0 12.49) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.10))))',
        f'    (fp_rect (start -1.3 -1.3) (end 1.3 11.46) (stroke (width 0.15) (type default)) (fill none) (layer "F.SilkS") (tstamp {uid("J2-body")}))',
        f'    (fp_text user "PIN 1" (at 2.8 0) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))) (tstamp {uid("J2-pin1")}))',
    ]
    connected: list[tuple[str, float, float]] = []
    for index, net in enumerate(nets, 1):
        dy = (index - 1) * 2.54
        shape = "rect" if index == 1 else "circle"
        lines.append(f'    (pad "{index}" thru_hole {shape} (at 0 {dy:.2f}) (size 1.7 1.7) (drill 1.0) (layers "*.Cu" "*.Mask"){net_clause(net)} (pinfunction "{net}") (pintype "passive") (tstamp {uid("J2-"+str(index))}))')
        connected.append((net, x, y + dy))
    lines.append("  )")
    return "\n".join(lines), connected


def mappings() -> dict[str, list[str | None]]:
    m: dict[str, list[str | None]] = {}
    # STM32G0B1CBT6 physical LQFP-48 order, verified against DS13560.
    m["U1"] = [
        None, None, None, "+3V3", "VDDA", "+3V3", "POWER_GND", None, None, "NRST",
        "ADC_IN1", "ADC_IN2", "ADC_IN3", "ADC_IN4", "DAC1_RAW", "DAC2_RAW",
        "ADC_IN5", "ADC_IN6", "ADC_IN7", "ADC_IN8", "BOOT0", "VBAT_MON", "V5_MON",
        "V3V3_MON", "RELAY2_GATE", "CAN_SILENT", "SENSOR_5V_EN", "FLEX_CAPTURE",
        "IGN_SENSE", "PULL_4K7_EN_N", "RELAY1_GATE", "SERVICE", "CAN_RX", "CAN_TX",
        "SWDIO", "SWCLK", "KLINE_ENABLE", "PULL_A0", "PULL_A1", "PULL_A2",
        "PULL_1K_EN_N", "EGT_SCK", "EGT_SO", "EGT_CS", "KTX", "KRX", None, None,
    ]
    m["U2"] = ["BUCK_BOOT", "POWER_GND", "BUCK_FB", "IGN", "VBAT_PROTECTED", "BUCK_SW"]
    m["U3"] = ["+5V", "POWER_GND", "+5V", None, "+3V3"]
    m["U4"] = ["SENSOR_5V_SW", "POWER_GND", "SENSOR_5V_OC", "SENSOR_5V_EN", "+5V"]
    y4051 = ["AIN5", "AIN7", None, "AIN8", "AIN6", None, "POWER_GND", "POWER_GND", "PULL_A2", "PULL_A1", "PULL_A0", "AIN4", "AIN1", "AIN2", "AIN3", "+5V"]
    m["U5"] = y4051.copy(); m["U5"][2] = "EXC_1K"; m["U5"][5] = "PULL_1K_EN_N"
    m["U6"] = y4051.copy(); m["U6"][2] = "EXC_4K7"; m["U6"][5] = "PULL_4K7_EN_N"
    m["U7"] = ["KRX", None, "+5V", "KTX", "POWER_GND", "K_LINE", "VBAT_PROTECTED", None]
    m["U8"] = ["CAN_TX", "POWER_GND", "+5V", "CAN_RX", "+3V3", "CAN_L_PROTECTED", "CAN_H_PROTECTED", "CAN_SILENT"]
    m["U9"] = ["DAC1_RAW", "AOUT1_OP", "AOUT1_OP", "POWER_GND", "DAC2_RAW", "AOUT2_OP", "AOUT2_OP", "+5V"]
    m["U10"] = ["POWER_GND", "EGT_K_POS", "EGT_K_NEG", "+3V3", "EGT_SCK", "EGT_CS", "EGT_SO", None]
    m["U11"] = [None, "FLEX_IN", "POWER_GND", "FLEX_CAPTURE", "+3V3"]
    m["F1"] = ["VBAT", "VBAT_FUSED"]; m["Q1"] = ["IGN", "VBAT_PROTECTED", "VBAT_FUSED", "VBAT_FUSED"]
    m["D1"] = ["VBAT_PROTECTED", "POWER_GND"]; m["C1"] = ["VBAT_PROTECTED", "POWER_GND"]
    m["D2"] = ["POWER_GND", "BUCK_SW"]; m["L1"] = ["BUCK_SW", "+5V"]; m["C2"] = ["+5V", "POWER_GND"]; m["C3"] = ["+3V3", "POWER_GND"]
    m["F2"] = ["SENSOR_5V_SW", "SENSOR_5V"]; m["FB1"] = ["+3V3", "VDDA"]
    m["R1"] = ["+5V", "EXC_1K"]; m["R2"] = ["+5V", "EXC_4K7"]
    m["R3"] = ["+5V", "PULL_1K_EN_N"]; m["R4"] = ["+5V", "PULL_4K7_EN_N"]
    m["R5"] = ["RELAY1_GATE", "POWER_GND"]; m["R6"] = ["RELAY2_GATE", "POWER_GND"]
    m["R7"] = ["AOUT1_OP", "AOUT1"]; m["R8"] = ["AOUT2_OP", "AOUT2"]
    m["R9"] = ["+3V3", "NRST"]; m["R10"] = ["BOOT0", "POWER_GND"]
    m["C4"] = ["+3V3", "POWER_GND"]; m["C5"] = ["+3V3", "POWER_GND"]; m["C6"] = ["VDDA", "POWER_GND"]
    m["TP1"] = ["SWDIO"]; m["TP2"] = ["SWCLK"]; m["TP3"] = ["NRST"]
    m["D3"] = ["CAN_H", "CAN_L"]; m["D4"] = ["K_LINE", "POWER_GND"]; m["D5"] = ["FLEX_IN", "POWER_GND"]
    m["D6"] = ["AOUT1", "POWER_GND"]; m["D7"] = ["AOUT2", "POWER_GND"]; m["JP1"] = ["CAN_H", "CAN_L"]
    m["L2"] = ["CAN_H_PROTECTED", "CAN_L_PROTECTED", "CAN_H", "CAN_L"]
    m["Q4"] = ["RELAY1_GATE", "RELAY_OUT1", "POWER_GND", "RELAY_OUT1", "RELAY_OUT1", "RELAY_OUT1", "RELAY_OUT1", "RELAY_OUT1"]
    m["Q5"] = ["RELAY2_GATE", "RELAY_OUT2", "POWER_GND", "RELAY_OUT2", "RELAY_OUT2", "RELAY_OUT2", "RELAY_OUT2", "RELAY_OUT2"]
    for i in range(1, 9):
        m[f"TVS{10+i}"] = [f"AIN{i}", "POWER_GND"]
        m[f"R{100+i}"] = [f"AIN{i}", f"ADC_IN{i}"]
        m[f"R{110+i}"] = [f"ADC_IN{i}", "SENSOR_GND"]
        m[f"C{100+i}"] = [f"ADC_IN{i}", "SENSOR_GND"]
    return m


def main() -> None:
    rows = list(csv.DictReader(BOM.open(encoding="utf-8", newline="")))
    maps = mappings()
    positions = {
        # Power tree, kept left and away from the sensor front end.
        "Q1": (25, 29), "F1": (32, 29), "D1": (39, 29), "C1": (46, 29),
        "U2": (25, 41), "D2": (32, 41), "L1": (39, 41), "C2": (32, 48),
        "U3": (25, 53), "C3": (25, 59), "U4": (32, 58), "F2": (35, 64),
        # MCU, pull-up multiplexers and their local support parts.
        "U5": (46, 68), "U6": (58, 68), "U1": (68, 84),
        "R1": (43, 76), "R2": (49, 76), "R3": (55, 76), "R4": (61, 76),
        "C4": (58, 94), "C5": (64, 94), "C6": (70, 94), "FB1": (76, 94),
        "R9": (58, 101), "R10": (64, 101), "TP1": (83, 82), "TP2": (83, 88), "TP3": (83, 94),
        # Vehicle communications and thermocouple interface.
        "U8": (91, 29), "L2": (100, 29), "D3": (108, 29), "JP1": (108, 35),
        "U7": (101, 40), "D4": (109, 40), "U10": (101, 50), "U11": (109, 55), "D5": (109, 62),
        # Analog and relay outputs stay together at the right edge.
        "U9": (101, 67), "R7": (97, 74), "R8": (105, 74), "D6": (97, 79), "D7": (105, 79),
        "Q4": (95, 90), "Q5": (108, 90), "R5": (95, 101), "R6": (108, 101),
    }
    for channel in range(1, 9):
        x = 43.0 + (channel - 1) * 6.5
        positions[f"TVS{10 + channel}"] = (x, 42)
        positions[f"R{100 + channel}"] = (x, 47)
        positions[f"R{110 + channel}"] = (x, 52)
        positions[f"C{100 + channel}"] = (x, 57)
    footprints: list[str] = []
    points: dict[str, list[tuple[float, float]]] = {}
    j_text, j_points = j1(); footprints.append(j_text)
    for net, x, y in j_points: points.setdefault(net, []).append((x, y))
    j2_text, j2_points = j2(); footprints.append(j2_text)
    for net, x, y in j2_points: points.setdefault(net, []).append((x, y))
    missing = [r["Reference"] for r in rows if r["Reference"] not in {"J1", "J2"} and r["Reference"] not in positions]
    if missing:
        raise RuntimeError(f"missing explicit placement for: {', '.join(missing)}")
    for row in rows:
        ref = row["Reference"]
        if ref in {"J1", "J2"}: continue
        pad_nets = maps.get(ref)
        if pad_nets is None:
            pad_nets = [None] if ref.startswith("TP") else [None, None]
        x, y = positions[ref]
        fp, fp_points = footprint(ref, row["Value"], x, y, pad_nets, row["Package"])
        footprints.append(fp)
        for net, px, py in fp_points: points.setdefault(net, []).append((px, py))

    segments: list[str] = []
    vias: list[str] = []
    for net_index, (net, pads) in enumerate(sorted(points.items(), key=lambda item: NET_ID[item[0]])):
        if len(pads) < 2 or net in {"POWER_GND", "SENSOR_GND"}: continue
        lane_y = 38.0 + (net_index % 56) * 1.05
        hub_x = 22.5 + (net_index % 2) * 91.0
        width = 0.6 if net in {"VBAT", "VBAT_FUSED", "VBAT_PROTECTED", "+5V", "SENSOR_5V", "RELAY_OUT1", "RELAY_OUT2"} else 0.25
        root_x, root_y = pads[0]
        for branch, (px, py) in enumerate(pads):
            vx = max(21.5, min(114.5, hub_x + branch * (0.18 if hub_x < 60 else -0.18)))
            segments.append(f'  (segment (start {px:.2f} {py:.2f}) (end {vx:.2f} {py:.2f}) (width {width}) (layer "F.Cu") (net {NET_ID[net]}) (tstamp {uid(net+"-a-"+str(branch))}))')
            segments.append(f'  (segment (start {vx:.2f} {py:.2f}) (end {vx:.2f} {lane_y:.2f}) (width {width}) (layer "F.Cu") (net {NET_ID[net]}) (tstamp {uid(net+"-b-"+str(branch))}))')
            vias.append(f'  (via (at {vx:.2f} {lane_y:.2f}) (size 0.8) (drill 0.4) (layers "F.Cu" "B.Cu") (net {NET_ID[net]}) (tstamp {uid(net+"-v-"+str(branch))}))')
            if branch:
                segments.append(f'  (segment (start {hub_x:.2f} {lane_y:.2f}) (end {vx:.2f} {lane_y:.2f}) (width {width}) (layer "B.Cu") (net {NET_ID[net]}) (tstamp {uid(net+"-c-"+str(branch))}))')

    header = ['(kicad_pcb (version 20240108) (generator pcbnew)', '  (general (thickness 1.6))', '  (paper "A4")',
              '  (layers (0 "F.Cu" signal) (31 "B.Cu" signal) (36 "B.SilkS" user "B.Silkscreen") (37 "F.SilkS" user "F.Silkscreen") (40 "Dwgs.User" user "User.Drawings") (41 "Cmts.User" user "User.Comments") (44 "Edge.Cuts" user) (46 "B.CrtYd" user "B.Courtyard") (47 "F.CrtYd" user "F.Courtyard") (48 "B.Fab" user) (49 "F.Fab" user))',
              '  (setup (pad_to_mask_clearance 0))', '  (net 0 "")']
    header += [f'  (net {NET_ID[n]} "{n}")' for n in NETS]
    tail = [
        '  (gr_text "NETCLASS POWER WIDTH 0.60MM" (at 33 103) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "NETCLASS CAN DIFF 0.25MM GAP 0.20MM" (at 60 103) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "NETCLASS EGT WIDTH 0.20MM NO VIAS" (at 90 103) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "NETCLASS ANALOG WIDTH 0.20MM" (at 55 100) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "NETCLASS RELAY WIDTH 0.60MM" (at 90 100) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "BOTTOM POWER_GND PLANE" (at 68 105) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "EGT COPPER KEEPOUT" (at 100 55) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "BUCK SWITCH NODE KEEPOUT" (at 31 54) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "PROTOTYPE REV B - ROUTING REVIEW" (at 40 22) (layer "F.SilkS") (effects (font (size 0.9 0.9) (thickness 0.15))))',
        '  (gr_text "PIN 1 VBAT" (at 48 23) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "CAN TERM DEFAULT OPEN" (at 92 28) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "SWDIO SWCLK TP_NRST" (at 70 67) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        '  (gr_text "ZONE_POWER ZONE_MCU_COMMS ZONE_ANALOG ZONE_EGT ZONE_OUTPUTS" (at 68 102) (layer "Cmts.User") (effects (font (size 0.7 0.7) (thickness 0.1))))',
        f'  (group "ROUTED_ANALOG_INPUTS" (id {uid("group-analog")}) (members))',
        f'  (group "ROUTED_POWER_TREE" (id {uid("group-power")}) (members))',
        f'  (group "ROUTED_COMMS_EGT_OUTPUTS" (id {uid("group-comms")}) (members))',
        f'  (zone (net {NET_ID["POWER_GND"]}) (net_name "POWER_GND") (layer "F.Cu") (tstamp {uid("gnd-zone-top")}) (hatch edge 0.5) (connect_pads yes (clearance 0.25)) (min_thickness 0.25) (fill yes (thermal_gap 0.3) (thermal_bridge_width 0.3) (island_removal_mode 0)) (polygon (pts (xy 20.5 20.5) (xy 115.5 20.5) (xy 115.5 105.5) (xy 20.5 105.5))))',
        f'  (zone (net {NET_ID["POWER_GND"]}) (net_name "POWER_GND") (layer "B.Cu") (tstamp {uid("gnd-zone")}) (hatch edge 0.5) (connect_pads (clearance 0.25)) (min_thickness 0.25) (fill yes (thermal_gap 0.3) (thermal_bridge_width 0.3) (island_removal_mode 0)) (polygon (pts (xy 20.5 20.5) (xy 115.5 20.5) (xy 115.5 105.5) (xy 20.5 105.5))))',
        f'  (gr_rect (start 20 20) (end 116 106) (stroke (width 0.25) (type default)) (fill none) (layer "Edge.Cuts") (tstamp {uid("outline")}))',
        ')',
    ]
    BOARD.write_text("\n".join(header + footprints + segments + vias + tail) + "\n", encoding="utf-8")
    print(f"generated {BOARD}: {len(footprints)} footprints, {sum(len(v) for v in points.values())} connected pads, {len(segments)} segments")


if __name__ == "__main__":
    main()

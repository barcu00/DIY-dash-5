"""Generate the project-local footprints referenced by the CAN I/O BOM."""
from __future__ import annotations

import csv
from pathlib import Path

from generate_can_io_routed_pcb import BOM, mappings


ROOT = Path(__file__).resolve().parents[1]
LIBRARY = ROOT / "hardware/can-io-module/lib/diy_dash_can_io.pretty"


def safe_name(package: str) -> str:
    return package.replace(" ", "_")


def coordinates(count: int) -> tuple[list[tuple[float, float]], list[tuple[float, float]]]:
    if count == 1:
        points = [(0.0, 0.0)]
    elif count == 2:
        points = [(-1.0, 0.0), (1.0, 0.0)]
    elif count <= 6:
        left = (count + 1) // 2
        points = [(-1.4, (i - (left - 1) / 2) * 1.0) for i in range(left)]
        points += [(1.4, ((count - left - 1) / 2 - i) * 1.0) for i in range(count - left)]
    elif count <= 16:
        half = count // 2
        points = [(-2.7, (i - (half - 1) / 2) * 1.27) for i in range(half)]
        points += [(2.7, ((count - half - 1) / 2 - i) * 1.27) for i in range(count - half)]
    else:
        side = count // 4
        points = [(-4.4, (i - (side - 1) / 2) * 0.5) for i in range(side)]
        points += [((i - (side - 1) / 2) * 0.5, 4.4) for i in range(side)]
        points += [(4.4, ((side - 1) / 2 - i) * 0.5) for i in range(side)]
        points += [(((side - 1) / 2 - i) * 0.5, -4.4) for i in range(count - 3 * side)]
    if count > 16:
        side = count // 4
        sizes = [(1.2, 0.25)] * side + [(0.25, 1.2)] * side
        sizes += [(1.2, 0.25)] * side + [(0.25, 1.2)] * (count - 3 * side)
    else:
        sizes = [(1.15, 0.75)] * count
    return points, sizes


def write_footprint(name: str, count: int, through_hole: bool = False) -> None:
    points, sizes = coordinates(count)
    body_x = 5.8 if count <= 16 else 9.0
    body_y = max(3.2, max((abs(y) for _, y in points), default=0) * 2 + 2) if count <= 16 else 9.0
    lines = [
        f'(footprint "{name}"',
        '  (version 20240108)',
        '  (generator pcbnew)',
        '  (layer "F.Cu")',
        f'  (fp_rect (start {-body_x/2:.2f} {-body_y/2:.2f}) (end {body_x/2:.2f} {body_y/2:.2f}) (stroke (width 0.15) (type default)) (fill none) (layer "F.SilkS"))',
        '  (fp_text reference "REF**" (at 0 -5) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))',
        '  (fp_text value "VAL**" (at 0 5) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.1))))',
    ]
    for index, ((x, y), (sx, sy)) in enumerate(zip(points, sizes), 1):
        shape = "rect" if index == 1 else ("circle" if through_hole else "roundrect")
        if through_hole:
            lines.append(f'  (pad "{index}" thru_hole {shape} (at {x:.2f} {y:.2f}) (size 1.7 1.7) (drill 1.0) (layers "*.Cu" "*.Mask"))')
        else:
            radius = ' (roundrect_rratio 0.2)' if shape == "roundrect" else ""
            lines.append(f'  (pad "{index}" smd {shape} (at {x:.2f} {y:.2f}) (size {sx:.2f} {sy:.2f}) (layers "F.Cu" "F.Paste" "F.Mask"){radius})')
    lines.append(')')
    (LIBRARY / f"{name}.kicad_mod").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    rows = list(csv.DictReader(BOM.open(encoding="utf-8", newline="")))
    pin_maps = mappings()
    package_counts: dict[str, int] = {}
    for row in rows:
        reference = row["Reference"]
        if reference in {"J1", "J2"}:
            continue
        pins = pin_maps.get(reference)
        if pins is None:
            pins = [None] if reference.startswith("TP") else [None, None]
        name = safe_name(row["Package"])
        previous = package_counts.setdefault(name, len(pins))
        if previous != len(pins):
            raise RuntimeError(f"package {name} has inconsistent pad counts: {previous}, {len(pins)}")
    for name, count in sorted(package_counts.items()):
        write_footprint(name, count)
    write_footprint("SWD_1x05_P2.54mm_Vertical", 5, through_hole=True)
    print(f"generated {len(package_counts) + 1} local CAN I/O footprints")


if __name__ == "__main__":
    main()

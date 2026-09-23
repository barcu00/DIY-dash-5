# Prototype manufacturing package

PROTOTYPE ONLY. The GitHub workflow creates review PDF/SVG files, Gerber layers, drill files, position CSV, ERC/DRC reports, and the checked-in BOM. These outputs are not production-approved.

Before ordering even one electrical prototype, print the PCB and connector at 1:1 and compare them with the exact purchased enclosure/header. Measure guide spacing, header seating, anchor holes, board thickness, lid height, seal compression, and cable-side mating clearance.

The target stack-up is 2-layer 1.6 mm FR-4, 1 oz copper. Do not change copper weight, finished thickness, via rules, or solder-mask expansion without rerunning DRC and reviewing connector fit. The bottom layer is intended to remain a substantially continuous POWER_GND plane.

The `generated` directory is created only by CI or the export script and is intentionally not the design authority. Source `.kicad_sch`, `.kicad_pcb`, BOM, and this release gate remain authoritative.

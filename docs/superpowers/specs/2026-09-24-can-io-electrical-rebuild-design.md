# CAN I/O module electrical rebuild design

## Status and objective

The existing revision proves the enclosure concept and a possible two-layer placement, but its schematic sheets contain descriptive text rather than an electrical netlist. A clean ERC on those files therefore does not prove electrical correctness. This rebuild replaces that prototype representation with a real, auditable KiCad design before any fabrication claim is made.

The objective is a two-layer CAN I/O module whose schematic is the sole electrical source of truth. Every populated PCB pad must originate from a real schematic symbol and a verified manufacturer pin number. GitHub Actions must reject missing symbols, missing pin mappings, schematic/PCB parity errors, unrouted nets, ERC errors, and DRC errors.

## Fixed product architecture

- MCU: STM32G0B1CBT6, LQFP-48.
- Vehicle connector: existing 24-pin FCI-style right-angle connector and approved external pin assignment.
- Programming: unpopulated 1x5, 2.54 mm SWD header with pin order `3V3_TARGET`, `SWDIO`, `SWCLK`, `NRST`, `GND`; pin 1 is marked on copper/silkscreen and the connector is accessible with the enclosure open.
- PCB: two copper layers, 1.6 mm FR-4, continuous ground return where routing permits.
- Functions: eight universal analog/resistive inputs, switchable 1 kOhm and 4.7 kOhm pull-ups to 5 V, protected sensor 5 V, CAN, L9613 K-line, Type-K EGT, Flex Fuel, two 0-5 V logger outputs, and two protected low-side relay outputs.

## Rebuild strategy

Three approaches were considered:

1. Patch the existing PCB only. This is rejected because it preserves the absence of an electrical source of truth.
2. Replace the MCU with a larger package and restart. This is not necessary unless the verified STM32 alternate-function assignment proves impossible, and it would increase cost and board area.
3. Rebuild from verified symbols and regenerate the two-layer board. This is selected. It preserves the approved product while making the design reviewable and testable.

## Schematic rules

- The root schematic contains the external connector, SWD connector, and hierarchical functional sheets.
- Every IC uses the exact physical pin numbers from its manufacturer data sheet.
- Power pins, exposed pads, no-connect pins, and required decoupling are explicit.
- No descriptive text may substitute for a symbol, wire, junction, hierarchical pin, or global label.
- The STM32 pin assignment must be documented in a machine-readable table including GPIO, peripheral alternate function, direction, voltage domain, and owning circuit.
- Reset behavior is safe without firmware: relay outputs off, pull-up multiplexers disabled, analog outputs at their minimum safe state, CAN in a defined state, and K-line not actively driven.

## Interface-specific corrections

### Power

The input fuse, reverse-polarity MOSFET, TVS, buck converter, 3.3 V regulator, sensor-rail switch, feedback networks, catch diode, inductor, input/output capacitors, and enable network are implemented exactly from their selected-device reference circuits. All regulator and MCU supply pins receive the required local capacitors.

### Analog inputs

AIN1-AIN8 use identical protected divider/filter networks. Both 74HCT4051 devices have real pin mappings and hardware-disable bias. Pull-up selection occurs on the connector side of each input divider. Firmware can never be the only mechanism preventing pull-up activation during reset.

### CAN

TJA1051T/3 uses its verified SO-8 mapping, 5 V transceiver supply, 3.3 V VIO, local decoupling, connector-side TVS, optional four-pad common-mode choke path, fitted zero-ohm bypasses when the choke is DNP, and optional 120 Ohm termination. The PCB may not contain a two-pad placeholder for a four-terminal common-mode choke.

### K-line

L9613 uses its verified SO-8 pinout and vehicle-side supply requirements. Any logic-level conditioning required between the transceiver and 3.3 V MCU is explicit. K-line protection and the passive/diagnostic-tool-safe state are shown electrically.

### EGT and Flex Fuel

MAX31855KASA+ is powered from 3.3 V with the manufacturer-required bypass and input conditioning. Thermocouple nets remain a matched, thermally symmetric pair. Flex Fuel uses a protected Schmitt front end and timer-capable STM32 input.

### Outputs

Both DAC paths use verified STM32 DAC-capable pins and a complete TLV9002-Q1 gain/filter/protection circuit. Relay drivers include gate series resistors, hardware pull-downs, connector protection, and a documented external-coil flyback strategy.

## PCB and footprint rules

- Manufacturer package drawings determine every production footprint; generic placement blocks are forbidden.
- The 24-pin connector remains provisional until checked against a physical sample and a 1:1 print.
- Power, CAN, EGT, analog, relay, and digital nets have explicit net classes.
- CAN is routed as a coupled pair. EGT is short, symmetric, and separated from the buck switch node and relay currents.
- Connector protection is placed before downstream circuitry.
- SWD is accessible and its pin-1 orientation is visible after assembly.

## Automated acceptance

GitHub Actions is the authoritative test environment. It must run:

1. Unit tests for exact IC pin maps and required circuit topology.
2. A schematic-content check requiring real symbols, wires/labels, junctions where needed, and nontrivial connectivity.
3. KiCad ERC with no unexplained violations.
4. Schematic-to-PCB parity validation with no missing or extra functional footprints.
5. KiCad DRC with no unexplained violations and zero unconnected pads.
6. Netlist-based checks for safe defaults, decoupling, protection order, SWD pin order, and all 24 external connector functions.
7. Schematic PDF, PCB render, BOM, position, Gerber, and drill exports as prototype artifacts.

Passing automation indicates design consistency, not vehicle qualification. Physical connector fit, bench bring-up, calibration, thermal tests, EMC/transient tests, and vehicle tests remain mandatory before production use.


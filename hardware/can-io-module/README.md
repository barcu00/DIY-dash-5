# DIY Dash 5 CAN I/O module

This KiCad 9 project is a low-cost, external automotive sensor and gateway module for DIY Dash 5. It combines eight configurable analog/resistive inputs, a Type-K EGT channel, Flex Fuel capture, BMW MS43 K-line, CAN, two 0–5 V signal outputs, and two protected low-side relay outputs.

## 24-pin connector

| Pin | Signal | Pin | Signal | Pin | Signal |
| ---: | --- | ---: | --- | ---: | --- |
| 1 | VBAT | 9 | AIN1 | 17 | EGT_K_POS |
| 2 | POWER_GND | 10 | AIN2 | 18 | EGT_K_NEG |
| 3 | IGN | 11 | AIN3 | 19 | RELAY_OUT1 |
| 4 | CAN_H | 12 | AIN4 | 20 | RELAY_OUT2 |
| 5 | CAN_L | 13 | AIN5 | 21 | FLEX_IN |
| 6 | K_LINE | 14 | AIN6 | 22 | AOUT1 |
| 7 | SENSOR_5V | 15 | AIN7 | 23 | AOUT2 |
| 8 | SENSOR_GND | 16 | AIN8 | 24 | SERVICE |

The analog outputs are only for ECU/logger inputs of at least 10 kOhm. Relay outputs are limited to 500 mA continuous each. This module is not a safety controller.

## Validation

GitHub Actions installs KiCad 9, runs the Python design contract, ERC, DRC with schematic parity, schematic PDF and PCB SVG exports, position export, and Gerber/drill generation. The canonical commands are in `.github/workflows/can-io-hardware.yml`; `scripts/export_can_io_hardware.ps1` mirrors them for a workstation with KiCad 9.

Fabrication outputs are prototypes only. Do not order a production batch until the exact purchased connector and enclosure have passed the mechanical fit gate described in `manufacturing/prototype-fabrication-notice.txt`.

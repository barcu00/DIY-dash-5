# ECUMaster EMU Black CAN Stream profile used by OpenDash v0.2

## Frozen source

OpenDash v0.2 implements the standard EMU Black CAN stream from the official **ECUMaster EMU Black User Manual**, document version **1.4**, firmware **2.169 or later**, published **09 July 2026**, section **6. CAN Stream**.

Official source: `https://www.ecumaster.com/files/EMU_BLACK/EMU_BLACK_manual.pdf`

Protocol defaults:

- CAN 2.0B standard identifiers
- default bitrate: 1 Mbit/s
- byte order: little-endian
- default base ID: `0x600`
- frames: base ID + 0 through base ID + 7 (`0x600..0x607`)

OpenDash normalizes pressure channels transmitted in kPa to **bar** in the Parameter Registry. Other channels are stored in the engineering units shown below.

## Base ID + 0 (`0x600`)

| Byte | Channel | Raw type | Conversion | Registry unit |
|---|---|---|---|---|
| 0..1 | RPM | uint16 LE | raw | rpm |
| 2 | TPS | uint8 | raw × 0.5 | % |
| 3 | IAT | int8 | raw | C |
| 4..5 | MAP | uint16 LE | raw / 100 | bar |
| 6..7 | Injector PW | uint16 LE | raw / 62 | ms |

## Base ID + 1 (`0x601`)

| Byte | Channel | Raw type | Conversion | Registry unit |
|---|---|---|---|---|
| 0..1 | AIN1 | uint16 LE | raw × 5 / 1024 | V |
| 2..3 | AIN2 | uint16 LE | raw × 5 / 1024 | V |
| 4..5 | AIN3 | uint16 LE | raw × 5 / 1024 | V |
| 6..7 | AIN4 | uint16 LE | raw × 5 / 1024 | V |

## Base ID + 2 (`0x602`)

| Byte | Channel | Raw type | Conversion | Registry unit |
|---|---|---|---|---|
| 0..1 | VSPD | uint16 LE | raw | km/h |
| 2 | BARO | uint8 | raw / 100 | bar |
| 3 | Oil Temperature | uint8 | raw | C |
| 4 | Oil Pressure | uint8 | raw / 16 | bar |
| 5 | Fuel Pressure | uint8 | raw / 16 | bar |
| 6..7 | CLT | int16 LE | raw | C |

## Base ID + 3 (`0x603`)

| Byte | Channel | Raw type | Conversion | Registry unit |
|---|---|---|---|---|
| 0 | Ignition Angle | int8 | raw × 0.5 | deg |
| 1 | Dwell Time | uint8 | raw × 0.05 | ms |
| 2 | Lambda | uint8 | raw / 128 | lambda |
| 3 | Lambda Correction | uint8 | raw × 0.5 | % |
| 4..5 | EGT1 | uint16 LE | raw | C |
| 6..7 | EGT2 | uint16 LE | raw | C |

## Base ID + 4 (`0x604`)

| Byte | Channel | Raw type | Conversion |
|---|---|---|---|
| 0 | Gear | uint8 | raw |
| 1 | ECU Temperature | int8 | raw C |
| 2..3 | Battery Voltage | uint16 LE | raw × 0.027 V |
| 4..5 | Error Flag | uint16 LE bitfield | see below |
| 6 | FLAGS1 | uint8 bitfield | see below |
| 7 | Ethanol Content | uint8 | raw % |

### Error Flag bits

| Bit | Registry semantic |
|---|---|
| 0 | CLT sensor error |
| 1 | IAT sensor error |
| 2 | MAP sensor error |
| 3 | wideband oxygen sensor error |
| 4 | EGT1 sensor error |
| 5 | EGT2 sensor error |
| 6 | EGT high alarm |
| 7 | knocking detected |
| 8 | flex-fuel sensor error |
| 9 | DBW error |
| 10 | fuel-pressure-relative error |

`EcuError` is also set to 1 when any documented Error Flag bit 0..10 is active.

### FLAGS1 bits

| Bit | Registry semantic |
|---|---|
| 0 | gear cut active |
| 1 | anti-lag active |
| 2 | launch control active |
| 3 | idle active |
| 4 | table set #2 active |
| 5 | traction-control intervention |
| 6 | pit limiter active |
| 7 | brake switch active |

## Base ID + 5 (`0x605`)

| Byte | Channel | Raw type | Conversion |
|---|---|---|---|
| 0 | DBW Position | uint8 | raw × 0.5 % |
| 1 | DBW Target | uint8 | raw × 0.5 % |
| 2..3 | TC DRPM RAW | int16 LE | raw |
| 4..5 | TC DRPM | uint16 LE | raw |
| 6 | TC Torque Reduction | uint8 | raw % |
| 7 | PIT Limiter Torque Reduction | uint8 | raw % |

## Base ID + 6 (`0x606`)

| Byte | Channel | Raw type | Conversion |
|---|---|---|---|
| 0..1 | AIN5 | uint16 LE | raw × 5 / 1024 V |
| 2..3 | AIN6 | uint16 LE | raw × 5 / 1024 V |
| 4 | OUTFLAGS1 | uint8 bitfield | see below |
| 5 | OUTFLAGS2 | uint8 bitfield | see below |
| 6 | OUTFLAGS3 | uint8 bitfield | see below |
| 7 | OUTFLAGS4 | uint8 bitfield | see below |

### OUTFLAGS1

Bits 0..4 are parametric outputs PO1..PO5. Bits 5..7 are virtual outputs VPO1..VPO3.

### OUTFLAGS2

Bits 0..7 are CAN switch states CANSW1..CANSW8.

### OUTFLAGS3

| Bit | Registry semantic |
|---|---|
| 0 | switch 1 |
| 1 | switch 2 |
| 2 | switch 3 |
| 3 | MUX switch 1 |
| 4 | MUX switch 2 |
| 5 | MUX switch 3 |
| 6 | launch-control map set |
| 7 | anti-lag map set |

### OUTFLAGS4

| Bit | Registry semantic |
|---|---|
| 0 | fuel pump |
| 1 | coolant fan |
| 2 | AC clutch |
| 3 | AC fan |
| 4 | nitrous |
| 5 | starter request |
| 6 | boost map set |
| 7 | reserved / unused |

## Base ID + 7 (`0x607`)

| Byte | Channel | Raw type | Conversion |
|---|---|---|---|
| 0..1 | Boost Target | uint16 LE | raw / 100 bar |
| 2 | PWM1 duty | uint8 | raw % |
| 3 | DSG Mode | low 4-bit enumeration | raw & 0x0F |
| 4 | Lambda Target | uint8 | raw × 0.01 lambda |
| 5 | PWM2 duty | uint8 | raw % |
| 6..7 | Fuel Used | uint16 LE | raw × 0.01 L |

DSG Mode enumeration from the manual:

- 2 = P
- 3 = R
- 4 = N
- 5 = D
- 6 = S
- 7 = M
- 15 = fault

## Decoder validity rules

- only the configured base ID through base ID + 7 are accepted;
- every standard stream frame must have DLC 8;
- an unrecognized ID or invalid DLC returns `false` and does not update registry data;
- accepted frames return `true` and stamp all values decoded from that frame with the supplied `now_ms`;
- raw CAN scaling remains confined to the ECUMaster decoder.

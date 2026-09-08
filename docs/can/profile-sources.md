# CAN Profile Source Ledger

This ledger records the exact source used to translate each compiled mapping.
It is documentation of receive-side decoding only; the dashboard transmits no
CAN frames.

| Profile ID | Status | Default | Source |
|---|---|---:|---|
| `ecumaster_emu_black` | verified | 1000 kbit/s | [EMU Black manual](https://www.ecumaster.com/files/EMU_BLACK/EMU_BLACK_manual.pdf), document 1.4, firmware 2.169+, published 2026-07-09, CAN Stream pp. 10-12 |
| `rusefi_verbose` | verified | 500 kbit/s | [rusEFI DBC](https://github.com/rusefi/rusefi/blob/71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32/firmware/controllers/can/rusEFI_CAN_verbose.dbc) and [transmitter](https://github.com/rusefi/rusefi/blob/71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32/firmware/controllers/can/can_verbose.cpp) |
| `maxxecu_default_1_3` | verified | 500 kbit/s | [MaxxECU Default CAN output](https://www.maxxecu.com/webhelp/can-default_maxxecu_protocol.html), protocol 1.3, 2020-09-29 |
| `haltech_broadcast_2_0` | verified | 1000 kbit/s | [Haltech ECU Broadcast CAN Protocol](https://support.haltech.com/portal/en/kb/articles/haltech-can-ecu-broadcast-protocol), document 2.0 |
| `speeduino_haltech` | verified | 500 kbit/s | [Speeduino CAN source](https://github.com/speeduino/speeduino/blob/5275fbaf82e57b364d9e0d0b4cbadbc33f95c668/speeduino/comms_CAN.cpp) and [header](https://github.com/speeduino/speeduino/blob/5275fbaf82e57b364d9e0d0b4cbadbc33f95c668/speeduino/comms_CAN.h) |
| `link_generic_dash_experimental` | experimental | 1000 kbit/s | [official Link channel list](https://kb.linkecu.com/acc-kb/latest/can-gauge-available-channels) plus [LinkGenericDash](https://github.com/AdaptiveEngineering/LinkGenericDash/tree/cd7426e872a40a812880e0b1c6af9c39391616c1) community decoder |
| `psa_c2_vts_engine_experimental` | experimental | 500 kbit/s | [PSA-RE](https://github.com/prototux/PSA-RE/tree/74294e99bd8f4decbfcdceabb11c7413dd977f4d/buses/AEE2004.full/HS.IS), with [archived cross-reference](https://github.com/prototux/PSA-CAN-RE-old/tree/d09c912bf6c53b9f24a6963358a75b447f40abc5) |

Gauge.S standalone definitions at revision
[`594208bf648dfd115fbfd639e5b7718451dffad5`](https://github.com/handmade0octopus/gauge.s-sorek.uk/tree/594208bf648dfd115fbfd639e5b7718451dffad5/definitions/2-Standalone)
were used only to compare translations.

## Implemented frame scope

- ECUMaster: standard IDs `0x600-0x607`, little-endian, DLC 8.
- rusEFI: standard IDs `0x200-0x20B`, little-endian, DLC 8, default base only.
- MaxxECU: standard IDs `0x520-0x537`, little-endian, DLC 8.
- Haltech: document 2.0 standard broadcast IDs, big-endian, DLC 8.
- Speeduino: the populated subset of its Haltech-compatible frames; bytes
  explicitly written as zero placeholders are unsupported.
- Link: standard ID `0x3E8`, DLC 8, byte 0 frame index, byte 1 zero, payload
  from byte 2. The ECU must be configured to transmit Generic Dash at this ID.
- PSA C2: standard HS.IS frames `0x208` (RPM, TPS) and `0x488` (coolant,
  oil, intake temperatures), DLC 8. No VAN, diagnostics, ABS frames, status
  flags, or uncertain signals are decoded.

## Verification boundary

Vendor/project protocol fixtures prove the translation in software. Link and
PSA remain experimental because their exact target-hardware stream has not been
captured in this project. The PSA source explicitly warns that its data is
reverse-engineered and may be incomplete. Before relying on the C2 profile,
capture the bus passively and compare values to an independent diagnostic
reading.

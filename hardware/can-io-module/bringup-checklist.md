# CAN I/O module prototype bring-up checklist

Required equipment: current-limited bench supply, DMM, oscilloscope, CAN interface with listen-only mode, SWD probe, precision voltage/resistance standards, Type-K thermocouple simulator, and protected relay dummy loads.

1. Inspect polarity, pin 1, solder bridges, unpopulated JP1 termination, enclosure clearance, and all connector-side protection.
2. Perform current-limited first power at 12.0 V with a 100 mA limit. Pass: no heating and consumption remains below the limit. Stop immediately on oscillation or current limiting.
3. Measure the protected battery node, 5 V rail (4.90–5.10 V), 3.3 V rail (3.23–3.37 V), VDDA, and SENSOR_5V with no external load.
4. Confirm SWD access, device identity, NRST operation, BOOT0 low, and watchdog recovery.
5. Verify mux reset safety: cycle slow and fast supply ramps while monitoring every AIN. No 1 kOhm or 4.7 kOhm pull-up may appear before firmware explicitly enables it.
6. Run a CAN silent test at 500 kbit/s first. Verify dominant/recessive levels, then enable normal mode with JP1 open. Fit termination only at a physical bus end.
7. Verify K-line high-impedance state before connecting BMW diagnostics. Confirm an external diagnostic tool still controls the bus; test 9.6 kbit/s before patched high-rate operation.
8. Perform analog calibration on AIN1–AIN8 at 0.0, 0.5, 2.5, 4.5, and 5.0 V and with precision resistors in both pull-up modes. Confirm open/short detection.
9. Test EGT with a thermocouple simulator at ambient, 400 °C, 800 °C, and 1000 °C. Repeat after warming the connector area and record residual offset.
10. Confirm output safe state: AOUT1/AOUT2 remain at 0 V and both relay gates remain off during reset, bootloader entry, watchdog reset, and missing-CAN timeout.
11. Calibrate both analog outputs into 10 kOhm and 100 kOhm loads at 0, 1, 2.5, 4, and 5 V. Stop if any output exceeds 5.1 V.
12. Test each relay with a fused relay dummy load, starting at 100 mA and ending at 500 mA. Verify flyback waveform and component temperature.
13. Validate reverse polarity, brownout, ignition wake/shutdown, and controlled automotive transient tests before any vehicle installation.
14. Complete the physical enclosure fit gate: 1:1 print, connector engagement, PCB guide fit, lid and seal clearance, harness retention, vibration, and splash/ingress checks.

Any failed item blocks vehicle connection and fabrication release.

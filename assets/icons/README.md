# OpenDash icon assets

OpenDash v0.2 uses compact 16x16 monochrome icon masks for dashboard tiles and picker previews.

Each icon family has a stable `IconId` and a two-letter high-contrast glyph stored as 16 rows of 16-bit pixels in `src/ui/icon_assets.cpp`. The representation is deliberately independent from LVGL so it can be unit-tested on the native target; the LVGL tile renderer consumes the same mask when drawing icons on the ESP32-S3 display.

Current families include RPM, speed, gear, throttle, boost/MAP, pressure, temperature, oil pressure/temperature, fuel pressure/fuel, lambda/AFR, injector, ignition, battery, ECU, analog, PWM, DBW, traction control, launch control, anti-lag, limiter, brake, fan, nitrous, CAN, warning, switches, outputs, starter, air-conditioning and lap/time.

The masks are 1-bit and therefore require only 32 bytes per 16x16 icon before compiler/linker overhead. They are intended for sharp, high-contrast motorsport presentation and can later be replaced family-by-family with richer pictograms without changing `IconId`, `IconCatalog`, `ParameterDescriptor` or saved tile configuration.

# RaceChrono BLE Monitor Integration Design

## Purpose

Add RaceChrono lap, sector, delta, GPS, and calculated channels to DIY Dash 5 as an optional Bluetooth Low Energy overlay. CAN or DEMO remains the sole engine-data source. RaceChrono supplements it with track-session data and never replaces, pauses, or reconfigures CAN reception.

Success means the driver can start a RaceChrono session, have the phone connect automatically to the dashboard, see live RaceChrono values in ordinary configurable tiles, and inspect connection/channel health without degrading the existing 50 Hz dashboard rendering or independent shift-light timing.

## Scope

The first release includes:

- RaceChrono DIY Monitor API support with the dashboard acting as a BLE peripheral and RaceChrono acting as the client.
- A persistent enable switch, automatic advertising, configuration, reconnect, and stale-data handling.
- A curated catalog of RaceChrono track, GPS, and calculated channels. Engine telemetry continues to come from CAN or DEMO.
- RaceChrono values in the existing tile parameter picker, including visibility, decimals, warnings, and per-layout persistence.
- One RaceChrono settings screen with `CONNECTION` and `CHANNELS` pages, matching the approved v4 mockup.
- Native tests for protocol framing, configuration sequencing, data decoding, stale-state behavior, settings migration, composite telemetry, and parameter selection.
- ESP32-S3 build verification and on-device BLE acceptance checks.

This release does not send CAN, GPS, or commands to RaceChrono; it only receives Monitor API values. It does not control session start/stop, video recording, or track selection. It does not add SD-card logging, Wi-Fi, cloud sync, or a third primary `DataSource`.

## User Flow

1. In `SETTINGS > DATA & CAN`, the user opens `RACECHRONO` and enables the integration.
2. The dashboard advertises the RaceChrono DIY service as `DIY DASH RC`.
3. Once, in RaceChrono, the user chooses `Settings > Add other device > RaceChrono DIY > Bluetooth LE > DIY DASH RC > Monitor`.
4. RaceChrono connects after the user starts a session. The dashboard removes any old Monitor API subscriptions, registers its channel catalog, and requests an initial update.
5. RaceChrono data becomes available in the ordinary tile editor. A RaceChrono tile shows unavailable data as `---`; CAN/DEMO tiles continue updating normally.
6. Later sessions reconnect automatically through RaceChrono's saved device configuration. The dashboard returns to advertising after a disconnect while the feature remains enabled.

## Architecture

The integration is a parallel overlay, not a new engine source:

```text
CAN or DEMO -> TelemetryManager -> VehicleState ---------+
                                                        +-> CompositeTelemetryView -> tiles, warnings, UI
RaceChrono BLE -> protocol state machine -> RC store ----+
```

`TelemetryManager` and `DataSource::{Can,Demo}` keep their present responsibilities. A new `RaceChronoTelemetry` owns only RaceChrono values and channel health. `CompositeTelemetryView` routes engine parameter IDs to `VehicleState` and RaceChrono parameter IDs to the overlay. Tile rendering and warning evaluation consume this read-only view, so RaceChrono values behave like other tile parameters without being copied into or masquerading as CAN data.

RaceChrono parameter IDs are appended to `ParameterId`; no existing numeric ID moves. This preserves saved tile selections. The registry gains metadata that identifies RaceChrono parameters and maps each one to a Monitor ID, equation, integer scale, unit, label, decimal default, and stale timeout.

## Components

### RaceChrono channel catalog

The catalog is compile-time, deterministic, and independent of the active CAN profile. Initial groups are:

- Timing: current lap number/time/distance, previous lap number/time, best lap number/time, comparison/reference lap number/time, live delta, lap-time gain, current sector time/distance, total race time/distance, and elapsed time.
- GPS: speed, altitude, bearing, latitude, longitude, satellites, fix type, accuracy, coordinate precision, altitude precision, 3D precision, and device update rate.
- Calculated: delta speed, lateral acceleration, longitudinal acceleration, combined acceleration, and lean angle.

The settings screen reports the actual catalog count; the `24 / 29` values in the visual mockup are illustrative, not a fixed protocol limit. Timing channels use `device(lap)`, GPS channels use `device(gps)`, and RaceChrono v8 calculated channels use `device(calc)`. `comparison_lap_time` is presented as `THEORETICAL / REFERENCE` because the API exposes the currently selected comparison lap rather than a separately guaranteed theoretical-best channel.

Each equation scales its result into a signed 32-bit integer with documented fixed-point precision. The receiver applies the inverse scale. Speed explicitly converts RaceChrono's internal meters per second to km/h before presentation. Latitude and longitude use the Monitor API's special degrees × 6,000,000 encoding.

### BLE transport

The hardware build pins `h2zero/NimBLE-Arduino` to version `2.5.1`. The ESP32-S3 exposes service `0x1FF8` with:

- configuration characteristic `0x0005`: `INDICATE | WRITE`;
- values characteristic `0x0006`: `WRITE_WITHOUT_RESPONSE`.

Advertising includes the service UUID and the device name `DIY DASH RC`. BLE callbacks never call LVGL, save NVS, or perform lengthy parsing. They copy bounded events into a fixed-capacity queue. `App::loop()` drains that queue with a per-iteration budget, advances the protocol state machine, and publishes decoded values.

The state machine is:

```text
DISABLED -> ADVERTISING -> CONNECTED -> CONFIGURING -> ACTIVE
                                  |            |          |
                                  +---------- ERROR     NO DATA
                                               \          /
                                                reconnect
```

After the client subscribes to indications, configuration sends `Remove all`, then each equation as one or more 17-byte payload fragments using `Add incomplete` and `Add complete`. Fragments are sequenced and bounded. ATT indication confirmation gates the next fragment; the app-level write result gates the next channel. After all possible channels are processed, the dashboard sends `Update all` and enters `ACTIVE` if at least one channel is valid. While active it requests `Update all` every 2 seconds so unchanged lap/session values are refreshed as well as change-driven live values.

### Protocol and channel errors

- A malformed `0x0006` write whose length is not divisible by five is rejected atomically and counted; no partial values are published.
- Unknown Monitor IDs are ignored and counted.
- Values are decoded as big-endian signed 32-bit integers, except for the documented latitude/longitude invalid sentinel.
- `Payload out-of-sequence` retries only the affected equation from fragment zero, with a bounded retry count.
- `Equation exception` marks only that channel `ERROR`; remaining channels continue configuring.
- Configuration timeouts retry with bounded backoff. Exhaustion returns to advertising after disconnect instead of blocking the main loop.
- A connected integration with no valid value packet for 2.5 seconds reports `NO DATA`. A channel not refreshed for 5 seconds becomes invalid and renders as `---`; the periodic `Update all` request prevents unchanged but available values from expiring.
- Disabling the feature stops advertising, disconnects the client, clears the overlay, and leaves CAN/DEMO untouched.

### Runtime status

`RaceChronoRuntimeStatus` exposes connection state, device name, last packet age, packets per second, configured/active/error channel counts, satellites, GPS fix type, GPS accuracy, and last protocol error. UI code receives a snapshot; it does not query NimBLE objects directly.

### Configuration and persistence

`AppConfig` schema becomes version 9 and adds `RaceChronoSettings` with an `enabled` flag. The default is disabled. Channel subscriptions are not individually persisted in this release because the agreed behavior is to request the complete curated catalog automatically.

A dedicated version-8 migration copies every existing setting and tile bank unchanged, then initializes RaceChrono disabled. Appending new `ParameterId` values and preserving all old enum values ensures existing tile selections remain valid. RaceChrono tile choices are saved through the existing deferred settings commit path exactly like other tile selections.

## UI Design

The existing six-card settings home remains unchanged to avoid shrinking cards or reintroducing scrolling artifacts. The `DATA & CAN` page gains a full-width `RACECHRONO` button in its unused lower area. That button opens a full-screen settings page with the approved v4 structure.

The RaceChrono screen has one title, back button, bottom navigation, and two equal tabs:

- `CONNECTION`: one continuous bordered panel with vertical rows for `ENABLED`, `CONNECTION`, `BLE DEVICE`, and `LAST DATA`; a two-column, two-row unboxed summary for BLE packets, active channels, satellites, and GPS accuracy; then signal/fix/update-rate text and `RESTART BLE`.
- `CHANNELS`: one filter control (`ALL`, `ACTIVE`, `NO DATA`, or `ERROR`), six fixed-height channel rows, and previous/next paging. Each row shows label, state, and current value.

There are no four-card horizontal rows. Global `box-sizing`, fixed panel bounds, minimum-width constraints, clipped long values, and a single framed panel prevent the horizontal overlap seen in earlier mockups. The UI remains English-only. Opening, changing tabs, leaving, reconnecting, or saving tile settings must use the existing safe screen-replacement/deferred-delete path and must not render dashboard data behind the settings screen.

The normal tile parameter picker adds a `RACECHRONO` category. RaceChrono parameters remain selectable whenever the feature is disabled or disconnected so layouts can be prepared in advance; their live value is simply unavailable. CAN capability filtering continues to apply only to engine parameters.

## Performance and Concurrency

- BLE callbacks perform bounded copies only; no LVGL or NVS calls occur from BLE context.
- Protocol buffers, event queues, channel state, and equations have compile-time maximum sizes. Runtime packet handling does not allocate per packet.
- The main loop drains a bounded number of BLE events before continuing CAN polling and display service.
- Dashboard rendering stays on the existing 50 Hz schedule. BLE packet arrival does not trigger a full screen redraw; it only updates the overlay snapshot consumed by the next normal tile refresh.
- Shift-light scheduling remains independent of UI and BLE work.
- Packet-rate and channel-status counters update at a low diagnostic cadence rather than every received value.

## Testing

Native tests cover:

- configuration command encoding, 17-byte equation fragmentation, sequence numbers, and command ordering;
- success, out-of-sequence retry, equation exception isolation, timeout, reconnect, and disable transitions;
- multiple five-byte value records, signed values, invalid lengths, unknown IDs, sentinel values, scale conversion, and stale invalidation;
- composite lookup proving CAN/DEMO engine values and RaceChrono values coexist and cannot overwrite each other;
- schema-8 to schema-9 migration preserving every prior field and tile bank;
- RaceChrono parameter descriptors, category filtering, formatting, warning evaluation, and tile persistence;
- settings paging/filter logic and layout bounds independent of LVGL.

Build checks run the full native suite and the `waveshare_5` firmware build. Hardware acceptance uses RaceChrono on both Android and iOS when available: discover the device, save it as a Monitor, start a session, confirm configuration, verify lap/delta/GPS values, stop/restart the session, verify reconnect, exercise `RESTART BLE`, and confirm CAN/DEMO plus shift-light behavior remains smooth during BLE traffic. RAM/heap diagnostics are captured before enabling BLE, while advertising, while active, and after disconnect; repeated reconnect cycles must not show monotonic heap loss.

## Acceptance Criteria

- RaceChrono can discover and connect to `DIY DASH RC` through the documented DIY Monitor flow.
- At least one supported channel reaching `ACTIVE` makes data available to tiles; one rejected equation does not break other channels.
- All catalog entries appear in the tile picker and persist after reboot.
- CAN or DEMO continues updating while RaceChrono is advertising, configuring, active, stale, disconnected, or disabled.
- Connection and channel pages match the approved v4 single-panel layout at 800 × 480 with no overlapping or overflowing controls.
- No BLE callback touches LVGL or NVS, and BLE processing cannot monopolize the main loop.
- Existing schema-8 user settings migrate without being reset.
- Native tests and the production ESP32-S3 build pass.

## Protocol References

- [Official RaceChrono BLE DIY API](https://github.com/aollin/racechrono-ble-diy-device)
- [RaceChrono equation reference](https://racechrono.com/support/equations)
- [RaceChrono equation identifiers](https://racechrono.com/support/equations/identifiers)
- [RaceChrono v8 calculated-device clarification](https://racechrono.com/forum/d/2354-2354)
- [NimBLE-Arduino 2.5.1 documentation](https://h2zero.github.io/NimBLE-Arduino/)

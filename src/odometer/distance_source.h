#pragma once

#include "odometer/odometer_model.h"

class CompositeTelemetryView;

// Picks the speed used for distance accumulation.
// - CAN SPEED: ECU vehicle speed.  WHEELS: mean of the valid wheel speeds.
// - GPS: RaceChrono GPS speed (real data even when the engine source is DEMO).
// - Simulated DEMO engine data never counts towards the odometer.
// - Optional GPS fallback when the selected CAN-based source is invalid.
SpeedSample selectDistanceSpeed(const OdometerSettings& settings,
                                const CompositeTelemetryView& state,
                                bool engine_data_simulated);

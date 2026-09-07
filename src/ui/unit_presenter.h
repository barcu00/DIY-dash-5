#pragma once

#include <cstdint>

#include "telemetry/parameter_registry.h"

enum class TemperatureUnit : uint8_t {
    Celsius,
    Fahrenheit,
};

enum class PressureUnit : uint8_t {
    Bar,
    Kpa,
    Psi,
};

enum class SpeedUnit : uint8_t {
    Kph,
    Mph,
};

enum class MixtureUnit : uint8_t {
    Lambda,
    Afr,
};

struct UnitSettings {
    TemperatureUnit temperature = TemperatureUnit::Celsius;
    PressureUnit pressure = PressureUnit::Bar;
    SpeedUnit speed = SpeedUnit::Kph;
    MixtureUnit mixture = MixtureUnit::Lambda;
    float stoich_afr = 14.7f;
};

struct PresentedValue {
    float value = 0.0f;
    const char* unit = "";
};

class UnitPresenter {
public:
    static PresentedValue present(ParameterId id, float native_value,
                                  const UnitSettings& settings);
    static float toNative(ParameterId id, float presented_value,
                          const UnitSettings& settings);
};

#include "unit_presenter.h"

namespace {
constexpr float kPsiPerBar = 14.5037738f;
constexpr float kMphPerKph = 0.621371192f;

const char* nativeUnitText(NativeUnit unit) {
    switch (unit) {
        case NativeUnit::Rpm: return "rpm";
        case NativeUnit::Bar: return "bar";
        case NativeUnit::Lambda: return "lambda";
        case NativeUnit::Percent: return "%";
        case NativeUnit::Celsius: return "°C";
        case NativeUnit::Volt: return "V";
        case NativeUnit::Kph: return "km/h";
        case NativeUnit::Gear: return "";
        case NativeUnit::Degrees: return "deg";
        case NativeUnit::Milliseconds: return "ms";
        case NativeUnit::GramsPerSecond: return "g/s";
        case NativeUnit::None: return "";
    }
    return "";
}
}  // namespace

PresentedValue UnitPresenter::present(ParameterId id, float native_value,
                                      const UnitSettings& settings) {
    const NativeUnit native_unit = parameterDescriptor(id).native_unit;
    switch (native_unit) {
        case NativeUnit::Celsius:
            if (settings.temperature == TemperatureUnit::Fahrenheit) {
                return {native_value * 1.8f + 32.0f, "°F"};
            }
            break;
        case NativeUnit::Bar:
            if (settings.pressure == PressureUnit::Kpa) {
                return {native_value * 100.0f, "kPa"};
            }
            if (settings.pressure == PressureUnit::Psi) {
                return {native_value * kPsiPerBar, "psi"};
            }
            break;
        case NativeUnit::Kph:
            if (settings.speed == SpeedUnit::Mph) {
                return {native_value * kMphPerKph, "mph"};
            }
            break;
        case NativeUnit::Lambda:
            if (settings.mixture == MixtureUnit::Afr) {
                return {native_value * settings.stoich_afr, "AFR"};
            }
            break;
        default:
            break;
    }
    return {native_value, nativeUnitText(native_unit)};
}

float UnitPresenter::toNative(ParameterId id, float presented_value,
                              const UnitSettings& settings) {
    switch (parameterDescriptor(id).native_unit) {
        case NativeUnit::Celsius:
            if (settings.temperature == TemperatureUnit::Fahrenheit) {
                return (presented_value - 32.0f) / 1.8f;
            }
            break;
        case NativeUnit::Bar:
            if (settings.pressure == PressureUnit::Kpa) {
                return presented_value / 100.0f;
            }
            if (settings.pressure == PressureUnit::Psi) {
                return presented_value / kPsiPerBar;
            }
            break;
        case NativeUnit::Kph:
            if (settings.speed == SpeedUnit::Mph) {
                return presented_value / kMphPerKph;
            }
            break;
        case NativeUnit::Lambda:
            if (settings.mixture == MixtureUnit::Afr &&
                settings.stoich_afr > 0.0f) {
                return presented_value / settings.stoich_afr;
            }
            break;
        default:
            break;
    }
    return presented_value;
}

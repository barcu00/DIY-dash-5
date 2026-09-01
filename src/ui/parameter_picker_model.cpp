#include "parameter_picker_model.h"

namespace {
constexpr uint16_t kCount = AppConfig::kParameterCount;

uint16_t toIndex(ParameterId id) {
    return static_cast<uint16_t>(id);
}

ParameterId fromIndex(uint16_t index) {
    return static_cast<ParameterId>(index);
}
}

ParameterId ParameterPickerModel::nextVisible(const AppConfig& config, ParameterId current) {
    const uint16_t start = toIndex(current) < kCount ? toIndex(current) : 0U;
    for (uint16_t step = 1; step <= kCount; ++step) {
        const uint16_t index = static_cast<uint16_t>((start + step) % kCount);
        if (config.parameter_visible[index]) return fromIndex(index);
    }
    return fromIndex(start);
}

ParameterId ParameterPickerModel::previousVisible(const AppConfig& config, ParameterId current) {
    const uint16_t start = toIndex(current) < kCount ? toIndex(current) : 0U;
    for (uint16_t step = 1; step <= kCount; ++step) {
        const uint16_t index = static_cast<uint16_t>((start + kCount - (step % kCount)) % kCount);
        if (config.parameter_visible[index]) return fromIndex(index);
    }
    return fromIndex(start);
}

bool ParameterPickerModel::setVisible(AppConfig& config, ParameterId parameter, bool visible) {
    const uint16_t index = toIndex(parameter);
    if (index >= kCount) return false;

    if (!visible && config.parameter_visible[index] && visibleCount(config) <= 1U) {
        return false;
    }

    config.parameter_visible[index] = visible;
    return true;
}

uint16_t ParameterPickerModel::visibleCount(const AppConfig& config) {
    uint16_t count = 0;
    for (bool visible : config.parameter_visible) {
        if (visible) ++count;
    }
    return count;
}

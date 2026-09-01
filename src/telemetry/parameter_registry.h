#pragma once

#include <array>
#include <cstdint>

#include "parameter_id.h"

enum class IconId : uint16_t;

struct ParameterDescriptor {
    ParameterId id;
    const char* name;
    const char* short_name;
    const char* unit;
    uint8_t decimals;
    float display_min;
    float display_max;
    bool is_boolean;
    IconId default_icon;
};

struct ParameterValue {
    float value = 0.0f;
    bool valid = false;
    uint32_t updated_ms = 0;
};

class ParameterRegistry {
public:
    ParameterRegistry();

    const ParameterDescriptor& descriptor(ParameterId id) const;
    const ParameterValue& value(ParameterId id) const;
    void set(ParameterId id, float value, uint32_t now_ms);
    void invalidateAll();

    void setPickerVisible(ParameterId id, bool visible);
    bool pickerVisible(ParameterId id) const;

    static constexpr uint16_t count() {
        return parameterIndex(ParameterId::Count);
    }

private:
    std::array<ParameterValue, count()> values_{};
    std::array<bool, count()> picker_visible_{};
};

#pragma once

#include <cstdint>

class SettingsScreenModel {
public:
    static constexpr uint8_t sectionCount() { return 7U; }
    static const char* sectionLabel(uint8_t index);
};

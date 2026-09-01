#include "settings_screen_model.h"

namespace {
constexpr const char* kSections[] = {
    "CAN",
    "DASH LAYOUT",
    "TRACK LAYOUT",
    "PARAMETERS",
    "WARNINGS",
    "FUEL / AFR",
    "SYSTEM",
};
}

const char* SettingsScreenModel::sectionLabel(uint8_t index) {
    return index < sectionCount() ? kSections[index] : "";
}

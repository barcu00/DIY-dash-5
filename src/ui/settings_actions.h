#pragma once
#include <cstdint>
enum SettingsAction : intptr_t {
    BrightnessPreview = 1,
    BrightnessCommit,
    SourceChanged,
    ProfileChanged,
    BitrateChanged,
    TimeoutDecrease,
    TimeoutIncrease,
    ShiftStartChanged,
    ShiftRedChanged,
    ShiftFlashChanged,
    ShiftMaxChanged,
    UnitsChanged,
    LayoutPresetChanged,
    RpmScaleChanged,
    WarningSoundChanged,
};

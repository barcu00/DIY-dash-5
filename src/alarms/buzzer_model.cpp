#include "buzzer_model.h"

void BuzzerModel::begin(uint32_t now_ms) {
    startup_ = true;
    startup_since_ = now_ms;
    sounding_warning_ = false;
}

bool BuzzerModel::update(uint32_t now_ms, bool warning_sound_enabled,
                         bool unacknowledged_warning) {
    if (startup_) {
        if (now_ms - startup_since_ < 200U) return true;
        startup_ = false;
    }
    const bool requested = warning_sound_enabled && unacknowledged_warning;
    if (requested && !sounding_warning_) warning_since_ = now_ms;
    sounding_warning_ = requested;
    return requested && (now_ms - warning_since_) % 600U < 150U;
}

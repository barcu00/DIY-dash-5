#pragma once
#include <cstdint>

class BuzzerModel {
public:
    void begin(uint32_t now_ms);
    bool update(uint32_t now_ms, bool warning_sound_enabled,
                bool unacknowledged_warning);
private:
    bool startup_ = false;
    bool startup_started_ = false;
    bool sounding_warning_ = false;
    uint32_t startup_since_ = 0;
    uint32_t warning_since_ = 0;
};

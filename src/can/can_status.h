#pragma once

#include <cstdint>

enum class CanStatus : uint8_t {
    Disabled,
    Waiting,
    Online,
    Offline,
    InitFailed,
};

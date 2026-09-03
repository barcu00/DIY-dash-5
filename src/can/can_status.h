#pragma once

#include <cstdint>

enum class CanStatus : uint8_t {
    Waiting,
    Online,
    Offline,
    InitFailed,
};

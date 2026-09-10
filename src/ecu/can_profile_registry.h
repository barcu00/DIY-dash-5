#pragma once

#include <cstddef>

#include "ecu/can_profile.h"

class CanProfileRegistry {
public:
    static std::size_t count();
    static const CanProfile* at(std::size_t index);
    static const CanProfile* find(const char* id);
};

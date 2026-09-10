#pragma once

#include <cstddef>
#include <cstdint>

namespace DisplayTuning {
constexpr uint16_t kBufferLines = 80U;

constexpr std::size_t bufferPixels(uint16_t width) {
    return static_cast<std::size_t>(width) * kBufferLines;
}
}  // namespace DisplayTuning

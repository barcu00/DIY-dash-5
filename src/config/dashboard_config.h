#pragma once

#include <cstdint>

namespace DashboardConfig {
constexpr uint32_t kCanBitrate = 1000000U;
constexpr uint32_t kCanTimeoutMs = 1500U;
constexpr bool kDemoEnabled = true;
constexpr uint8_t kCanTxGpio = 15U;
constexpr uint8_t kCanRxGpio = 16U;

constexpr float kCltWarningC = 105.0f;
constexpr float kCltCriticalC = 115.0f;
constexpr float kIatWarningC = 55.0f;
constexpr float kIatCriticalC = 70.0f;
constexpr float kOilPressureWarningBar = 1.2f;
constexpr float kOilPressureCriticalBar = 0.8f;
constexpr float kLeanLambdaWarning = 1.08f;
constexpr float kLeanLambdaCritical = 1.15f;
constexpr float kLeanLoadMapBar = 0.9f;
constexpr float kLeanLoadTpsPercent = 70.0f;
constexpr float kBatteryWarningV = 12.0f;
constexpr float kBatteryCriticalV = 11.0f;
}

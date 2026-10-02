#pragma once

#include <cstdint>

#include "board/hardware_profile.h"

namespace DashboardConfig {
constexpr uint32_t kCanBitrate = 1000000U;
constexpr uint32_t kCanTimeoutMs = 1500U;
constexpr uint32_t kUiUpdateIntervalMs = 25U;
constexpr bool kDemoEnabled = true;
constexpr uint8_t kCanTxGpio = currentHardwareProfile().can_tx_gpio;
constexpr uint8_t kCanRxGpio = currentHardwareProfile().can_rx_gpio;

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

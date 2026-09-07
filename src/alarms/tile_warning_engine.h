#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"

enum class TileAlarmPhase : uint8_t {
    Safe,
    Pending,
    ActiveUnacknowledged,
    ActiveAcknowledged,
};

struct WarningModalData {
    TileAddress address{};
    ParameterId parameter = ParameterId::Rpm;
    float current_native = 0.0f;
    float threshold_native = 0.0f;
    uint8_t remaining_count = 0U;
};

class TileWarningEngine {
public:
    void evaluate(const AppConfig& config, const VehicleState& state,
                  uint32_t now_ms);
    std::optional<WarningModalData> nextModal() const;
    void acknowledge(TileAddress address);
    bool isHighlighted(TileAddress address) const;

private:
    static constexpr std::size_t kRuntimeCount =
        AppConfig::kDashTileCount + AppConfig::kTrackTileCount;

    struct RuntimeWarning {
        TileAlarmPhase phase = TileAlarmPhase::Safe;
        bool signature_valid = false;
        ParameterId parameter = ParameterId::Rpm;
        WarningDirection direction = WarningDirection::Above;
        float threshold_native = 0.0f;
        float hysteresis_native = 0.0f;
        uint16_t delay_ms = 0U;
        uint32_t pending_since_ms = 0U;
        float current_native = 0.0f;
    };

    static std::optional<std::size_t> indexOf(TileAddress address);
    static TileAddress addressOf(std::size_t index);
    static const TileConfig& tileAt(const AppConfig& config,
                                    std::size_t index);
    static bool sameSignature(const RuntimeWarning& runtime,
                              const TileConfig& tile);
    static void setSignature(RuntimeWarning& runtime, const TileConfig& tile);
    static bool breached(const RuntimeWarning& runtime, float value);
    static bool returnedSafe(const RuntimeWarning& runtime, float value);
    static float priority(const RuntimeWarning& runtime);

    std::array<RuntimeWarning, kRuntimeCount> runtime_{};
};

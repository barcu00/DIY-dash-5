#include "tile_warning_engine.h"

#include <algorithm>
#include <cmath>

std::optional<std::size_t> TileWarningEngine::indexOf(TileAddress address) {
    if (address.page == PageId::Dash &&
        address.slot < AppConfig::kDashTileCount) {
        return address.slot;
    }
    if (address.page == PageId::Track &&
        address.slot < AppConfig::kTrackTileCount) {
        return AppConfig::kDashTileCount + address.slot;
    }
    return std::nullopt;
}

TileAddress TileWarningEngine::addressOf(std::size_t index) {
    if (index < AppConfig::kDashTileCount) {
        return {PageId::Dash, static_cast<uint8_t>(index)};
    }
    return {PageId::Track,
            static_cast<uint8_t>(index - AppConfig::kDashTileCount)};
}

const TileConfig& TileWarningEngine::tileAt(const AppConfig& config,
                                            std::size_t index) {
    if (index < AppConfig::kDashTileCount) {
        return config.dash_tiles[index];
    }
    return config.track_tiles[index - AppConfig::kDashTileCount];
}

bool TileWarningEngine::sameSignature(const RuntimeWarning& runtime,
                                      const TileConfig& tile) {
    return runtime.signature_valid && runtime.parameter == tile.parameter &&
           runtime.direction == tile.warning.direction &&
           runtime.threshold_native == tile.warning.threshold_native &&
           runtime.hysteresis_native == tile.warning.hysteresis_native &&
           runtime.delay_ms == tile.warning.delay_ms;
}

void TileWarningEngine::setSignature(RuntimeWarning& runtime,
                                     const TileConfig& tile) {
    runtime.phase = TileAlarmPhase::Safe;
    runtime.signature_valid = true;
    runtime.parameter = tile.parameter;
    runtime.direction = tile.warning.direction;
    runtime.threshold_native = tile.warning.threshold_native;
    runtime.hysteresis_native = tile.warning.hysteresis_native;
    runtime.delay_ms = tile.warning.delay_ms;
    runtime.pending_since_ms = 0U;
    runtime.current_native = 0.0f;
}

bool TileWarningEngine::breached(const RuntimeWarning& runtime, float value) {
    return runtime.direction == WarningDirection::Above
               ? value > runtime.threshold_native
               : value < runtime.threshold_native;
}

bool TileWarningEngine::returnedSafe(const RuntimeWarning& runtime,
                                     float value) {
    return runtime.direction == WarningDirection::Above
               ? value <= runtime.threshold_native - runtime.hysteresis_native
               : value >= runtime.threshold_native + runtime.hysteresis_native;
}

float TileWarningEngine::priority(const RuntimeWarning& runtime) {
    const float excursion = runtime.direction == WarningDirection::Above
                                ? runtime.current_native - runtime.threshold_native
                                : runtime.threshold_native - runtime.current_native;
    return std::max(0.0f, excursion) /
           std::max(std::fabs(runtime.threshold_native), 1.0f);
}

void TileWarningEngine::evaluate(const AppConfig& config,
                                 const VehicleState& state,
                                 uint32_t now_ms) {
    for (std::size_t index = 0U; index < runtime_.size(); ++index) {
        const TileConfig& tile = tileAt(config, index);
        RuntimeWarning& runtime = runtime_[index];

        if (!tile.warning.enabled ||
            static_cast<std::size_t>(tile.parameter) >= parameterCount()) {
            runtime = RuntimeWarning{};
            continue;
        }
        if (!sameSignature(runtime, tile)) {
            setSignature(runtime, tile);
        }

        const SignalValue& signal = state.get(tile.parameter);
        if (!signal.valid) {
            runtime.phase = TileAlarmPhase::Safe;
            runtime.pending_since_ms = 0U;
            continue;
        }
        runtime.current_native = signal.value;

        if (runtime.phase == TileAlarmPhase::Safe) {
            if (!breached(runtime, signal.value)) {
                continue;
            }
            if (runtime.delay_ms == 0U) {
                runtime.phase = TileAlarmPhase::ActiveUnacknowledged;
            } else {
                runtime.phase = TileAlarmPhase::Pending;
                runtime.pending_since_ms = now_ms;
            }
            continue;
        }

        if (runtime.phase == TileAlarmPhase::Pending) {
            if (!breached(runtime, signal.value)) {
                runtime.phase = TileAlarmPhase::Safe;
                runtime.pending_since_ms = 0U;
            } else if (now_ms - runtime.pending_since_ms >= runtime.delay_ms) {
                runtime.phase = TileAlarmPhase::ActiveUnacknowledged;
            }
            continue;
        }

        if (returnedSafe(runtime, signal.value)) {
            runtime.phase = TileAlarmPhase::Safe;
            runtime.pending_since_ms = 0U;
        }
    }
}

std::optional<WarningModalData> TileWarningEngine::nextModal() const {
    std::size_t unacknowledged_count = 0U;
    std::optional<std::size_t> best_index;
    float best_priority = -1.0f;

    for (std::size_t index = 0U; index < runtime_.size(); ++index) {
        const RuntimeWarning& runtime = runtime_[index];
        if (runtime.phase != TileAlarmPhase::ActiveUnacknowledged) {
            continue;
        }
        ++unacknowledged_count;
        const float candidate_priority = priority(runtime);
        if (!best_index.has_value() || candidate_priority > best_priority) {
            best_index = index;
            best_priority = candidate_priority;
        }
    }

    if (!best_index.has_value()) {
        return std::nullopt;
    }
    const RuntimeWarning& best = runtime_[*best_index];
    return WarningModalData{
        addressOf(*best_index), best.parameter, best.current_native,
        best.threshold_native,
        static_cast<uint8_t>(unacknowledged_count - 1U)};
}

void TileWarningEngine::acknowledge(TileAddress address) {
    const std::optional<std::size_t> index = indexOf(address);
    if (index.has_value() &&
        runtime_[*index].phase == TileAlarmPhase::ActiveUnacknowledged) {
        runtime_[*index].phase = TileAlarmPhase::ActiveAcknowledged;
    }
}

bool TileWarningEngine::isHighlighted(TileAddress address) const {
    const std::optional<std::size_t> index = indexOf(address);
    if (!index.has_value()) {
        return false;
    }
    const TileAlarmPhase phase = runtime_[*index].phase;
    return phase == TileAlarmPhase::ActiveUnacknowledged ||
           phase == TileAlarmPhase::ActiveAcknowledged;
}

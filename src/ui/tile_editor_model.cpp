#include "tile_editor_model.h"

#include <cmath>
#include <cstddef>

namespace {
const TileConfig* tileAt(const AppConfig& config, TileAddress address) {
    if (address.page == PageId::Dash &&
        address.slot < config.dash_tiles.size()) {
        return &config.dash_tiles[address.slot];
    }
    if (address.page == PageId::Track &&
        address.slot < config.track_tiles.size()) {
        return &config.track_tiles[address.slot];
    }
    return nullptr;
}

TileConfig* tileAt(AppConfig& config, TileAddress address) {
    if (address.page == PageId::Dash &&
        address.slot < config.dash_tiles.size()) {
        return &config.dash_tiles[address.slot];
    }
    if (address.page == PageId::Track &&
        address.slot < config.track_tiles.size()) {
        return &config.track_tiles[address.slot];
    }
    return nullptr;
}

bool validDraft(const TileConfig& tile) {
    return static_cast<std::size_t>(tile.parameter) < parameterCount() &&
           tile.decimals <= 3U &&
           static_cast<uint8_t>(tile.warning.direction) <=
               static_cast<uint8_t>(WarningDirection::Below) &&
           std::isfinite(tile.warning.threshold_native) &&
           std::isfinite(tile.warning.hysteresis_native) &&
           tile.warning.threshold_native >= 0.0f &&
           tile.warning.threshold_native <= 999.0f &&
           tile.warning.hysteresis_native >= 0.0f &&
           tile.warning.hysteresis_native <= 999.0f &&
           tile.warning.delay_ms <= 10000U &&
           std::isfinite(tile.temperature_bar.minimum_native) &&
           std::isfinite(tile.temperature_bar.ready_native) &&
           std::isfinite(tile.temperature_bar.maximum_native) &&
           tile.temperature_bar.minimum_native >= -999.0f &&
           tile.temperature_bar.minimum_native <
               tile.temperature_bar.ready_native &&
           tile.temperature_bar.ready_native <
               tile.temperature_bar.maximum_native &&
           tile.temperature_bar.maximum_native <= 999.0f;
}
}  // namespace

bool TileEditorModel::open(TileAddress address, const AppConfig& config) {
    const TileConfig* tile = tileAt(config, address);
    if (tile == nullptr) {
        return false;
    }
    draft_ = {address, *tile};
    original_parameter_ = tile->parameter;
    open_ = true;
    return true;
}

void TileEditorModel::cancel() {
    open_ = false;
}

bool TileEditorModel::isOpen() const {
    return open_;
}

const TileEditorDraft& TileEditorModel::draft() const {
    return draft_;
}

void TileEditorModel::setParameter(ParameterId parameter) {
    if (open_ && draft_.tile.parameter != parameter) {
        draft_.tile.parameter = parameter;
        draft_.tile.temperature_bar = defaultTemperatureBarConfig(parameter);
    }
}

void TileEditorModel::setVisible(bool visible) {
    if (open_) {
        draft_.tile.visible = visible;
    }
}

void TileEditorModel::setDecimals(uint8_t decimals) {
    if (open_) {
        draft_.tile.decimals = decimals;
    }
}

void TileEditorModel::setWarning(const TileWarningConfig& warning) {
    if (open_) {
        draft_.tile.warning = warning;
    }
}

void TileEditorModel::setTemperatureBar(
    const TemperatureBarConfig& temperature_bar) {
    if (open_) {
        draft_.tile.temperature_bar = temperature_bar;
    }
}

bool TileEditorModel::applyTo(AppConfig& config) {
    TileConfig* destination = tileAt(config, draft_.address);
    if (!open_ || destination == nullptr || !validDraft(draft_.tile)) {
        return false;
    }

    TileConfig candidate = draft_.tile;
    if (candidate.parameter != original_parameter_) {
        candidate.warning.enabled = false;
    }
    *destination = candidate;
    open_ = false;
    return true;
}

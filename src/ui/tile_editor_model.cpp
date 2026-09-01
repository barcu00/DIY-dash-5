#include "tile_editor_model.h"

#include <cstring>

namespace {
bool supportsAfr(ParameterId parameter) {
    return parameter == ParameterId::Lambda || parameter == ParameterId::LambdaTarget;
}
}

void TileEditorModel::setParameter(TileConfig& tile, ParameterId parameter) {
    tile.parameter = parameter;
    if (!supportsAfr(parameter)) {
        tile.value_format = ValueFormatMode::Native;
    }
}

void TileEditorModel::setVisible(TileConfig& tile, bool visible) {
    tile.visible = visible;
}

void TileEditorModel::setCustomLabel(TileConfig& tile, const char* label) {
    tile.custom_label.fill('\0');
    if (label == nullptr || label[0] == '\0') {
        tile.custom_label_enabled = false;
        return;
    }

    std::strncpy(tile.custom_label.data(), label, tile.custom_label.size() - 1U);
    tile.custom_label.back() = '\0';
    tile.custom_label_enabled = true;
}

void TileEditorModel::clearCustomLabel(TileConfig& tile) {
    tile.custom_label.fill('\0');
    tile.custom_label_enabled = false;
}

void TileEditorModel::useDefaultIcon(TileConfig& tile) {
    tile.icon_enabled = true;
    tile.icon = 0U;
}

void TileEditorModel::useCustomIcon(TileConfig& tile, IconId icon) {
    tile.icon_enabled = true;
    tile.icon = static_cast<uint16_t>(icon);
}

void TileEditorModel::disableIcon(TileConfig& tile) {
    tile.icon_enabled = false;
}

bool TileEditorModel::setAfrMode(TileConfig& tile, bool enabled) {
    if (enabled && !supportsAfr(tile.parameter)) {
        tile.value_format = ValueFormatMode::Native;
        return false;
    }

    tile.value_format = enabled ? ValueFormatMode::Afr : ValueFormatMode::Native;
    return true;
}

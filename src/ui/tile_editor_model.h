#pragma once

#include "settings/app_config.h"
#include "ui/icon_id.h"

class TileEditorModel {
public:
    static void setParameter(TileConfig& tile, ParameterId parameter);
    static void setVisible(TileConfig& tile, bool visible);

    static void setCustomLabel(TileConfig& tile, const char* label);
    static void clearCustomLabel(TileConfig& tile);

    // icon == 0 with icon_enabled=true means use the parameter's catalog default.
    static void useDefaultIcon(TileConfig& tile);
    static void useCustomIcon(TileConfig& tile, IconId icon);
    static void disableIcon(TileConfig& tile);

    // AFR presentation is meaningful only for Lambda/LambdaTarget parameters.
    static bool setAfrMode(TileConfig& tile, bool enabled);
};

#include "app_config_migration.h"

bool AppConfigMigration::fromV2(const LegacyAppConfigV2& legacy, AppConfig& out) {
    if (legacy.schema_version != LegacyAppConfigV2::kSchemaVersion) {
        return false;
    }

    // Initialize the new-only DASH2/DASH3 fields and all other defaults in-place,
    // then overwrite every field that existed in schema v2. This avoids a large
    // AppConfig temporary on the ESP32 Arduino task stack.
    AppConfig::resetToDefaults(out);

    out.data_source = legacy.data_source;
    out.can_bitrate = legacy.can_bitrate;
    out.ecumaster_base_id = legacy.ecumaster_base_id;
    out.can_timeout_ms = legacy.can_timeout_ms;
    out.lambda_format = legacy.lambda_format;
    out.stoich_afr = legacy.stoich_afr;

    out.dash_tiles[0] = legacy.tiles;
    out.track_tiles = legacy.track_tiles;
    out.parameter_visible = legacy.parameter_visible;
    out.warnings = legacy.warnings;
    out.schema_version = AppConfig::kSchemaVersion;
    out.validate();
    return true;
}

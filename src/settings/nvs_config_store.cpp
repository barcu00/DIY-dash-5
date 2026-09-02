#include "nvs_config_store.h"

#include "app_config_migration.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
#include <new>
#endif

bool NvsConfigStore::load(AppConfig& config) {
#if defined(ARDUINO_ARCH_ESP32)
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) {
        AppConfig::resetToDefaults(config);
        return false;
    }

    const size_t length = prefs.getBytesLength(kBlobKey);

    if (length == sizeof(AppConfig)) {
        // Read the current schema directly into the persistent application object.
        // AppConfig contains warning curves for all parameters and is intentionally
        // too large to duplicate on the Arduino task stack.
        const size_t read = prefs.getBytes(kBlobKey, &config, sizeof(config));
        prefs.end();

        if (read != sizeof(config) || !config.validSchema()) {
            AppConfig::resetToDefaults(config);
            return false;
        }

        config.validate();
        return true;
    }

    if (length == sizeof(LegacyAppConfigV2)) {
        // Never put the legacy structure on the task stack. It contains the same
        // 90 WarningConfig records that previously caused stack corruption when
        // AppConfig was copied from LVGL callbacks/startup code.
        auto* legacy = new (std::nothrow) LegacyAppConfigV2;
        if (legacy == nullptr) {
            prefs.end();
            AppConfig::resetToDefaults(config);
            return false;
        }

        const size_t read = prefs.getBytes(kBlobKey, legacy, sizeof(*legacy));
        prefs.end();
        const bool migrated = read == sizeof(*legacy) && AppConfigMigration::fromV2(*legacy, config);
        delete legacy;

        if (!migrated) {
            AppConfig::resetToDefaults(config);
            return false;
        }

        // Best-effort in-place schema upgrade. The migrated live configuration is
        // still usable for this boot even if the flash write fails.
        save(config);
        return true;
    }

    prefs.end();
    AppConfig::resetToDefaults(config);
    return false;
#else
    AppConfig::resetToDefaults(config);
    return false;
#endif
}

bool NvsConfigStore::save(const AppConfig& config) {
#if defined(ARDUINO_ARCH_ESP32)
    // Callers validate the live AppConfig before saving. Do not make a complete
    // stack copy here: the WarningConfig records make AppConfig larger than a
    // typical ESP32 Arduino task stack budget.
    if (!config.validSchema()) {
        return false;
    }

    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) {
        return false;
    }
    const size_t written = prefs.putBytes(kBlobKey, &config, sizeof(config));
    prefs.end();
    return written == sizeof(config);
#else
    (void)config;
    return false;
#endif
}

bool NvsConfigStore::clear() {
#if defined(ARDUINO_ARCH_ESP32)
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) {
        return false;
    }
    const bool ok = prefs.clear();
    prefs.end();
    return ok;
#else
    return false;
#endif
}

#include "nvs_config_store.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
#endif

bool NvsConfigStore::load(AppConfig& config) {
#if defined(ARDUINO_ARCH_ESP32)
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) {
        AppConfig::resetToDefaults(config);
        return false;
    }

    const size_t length = prefs.getBytesLength(kBlobKey);
    if (length != sizeof(AppConfig)) {
        prefs.end();
        AppConfig::resetToDefaults(config);
        return false;
    }

    // AppConfig contains the warning configuration for every parameter and is
    // intentionally large. Reading into a second local AppConfig used more than
    // a typical Arduino loop-task stack and could corrupt/reset the ESP32 during
    // startup after a configuration had been saved. Read directly into the
    // persistent application object and fall back to defaults if validation fails.
    const size_t read = prefs.getBytes(kBlobKey, &config, sizeof(config));
    prefs.end();

    if (read != sizeof(config) || !config.validSchema()) {
        AppConfig::resetToDefaults(config);
        return false;
    }

    config.validate();
    return true;
#else
    AppConfig::resetToDefaults(config);
    return false;
#endif
}

bool NvsConfigStore::save(const AppConfig& config) {
#if defined(ARDUINO_ARCH_ESP32)
    // Callers validate the live AppConfig before saving. Do not make a complete
    // stack copy here: the 90 WarningConfig records make AppConfig larger than a
    // typical ESP32 Arduino task stack budget and the copy happened from LVGL
    // button callbacks, which manifested as resets/corrupted duplicated graphics.
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

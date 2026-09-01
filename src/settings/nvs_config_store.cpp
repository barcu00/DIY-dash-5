#include "nvs_config_store.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
#endif

bool NvsConfigStore::load(AppConfig& config) {
#if defined(ARDUINO_ARCH_ESP32)
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) {
        config = AppConfig::defaults();
        return false;
    }

    const size_t length = prefs.getBytesLength(kBlobKey);
    if (length != sizeof(AppConfig)) {
        prefs.end();
        config = AppConfig::defaults();
        return false;
    }

    AppConfig loaded{};
    const size_t read = prefs.getBytes(kBlobKey, &loaded, sizeof(loaded));
    prefs.end();

    if (read != sizeof(loaded) || !loaded.validSchema()) {
        config = AppConfig::defaults();
        return false;
    }

    loaded.validate();
    config = loaded;
    return true;
#else
    config = AppConfig::defaults();
    return false;
#endif
}

bool NvsConfigStore::save(const AppConfig& config) {
#if defined(ARDUINO_ARCH_ESP32)
    AppConfig stored = config;
    stored.schema_version = AppConfig::kSchemaVersion;
    stored.validate();

    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) {
        return false;
    }
    const size_t written = prefs.putBytes(kBlobKey, &stored, sizeof(stored));
    prefs.end();
    return written == sizeof(stored);
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

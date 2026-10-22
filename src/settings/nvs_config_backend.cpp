#include "nvs_config_backend.h"

#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

// Preferences::begin() fails forever when the dedicated partition reports
// NO_FREE_PAGES or NEW_VERSION_FOUND (e.g. after an IDF upgrade or a
// truncated write). Recover once per boot by erasing only "dashcfg"; the
// config then falls back to defaults instead of being unsaveable.
bool ensureNvsPartition(const char* label) {
    static bool ready = false;
    if (ready) return true;
    esp_err_t result = nvs_flash_init_partition(label);
    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.printf("[CONFIG] NVS partition %s unusable (%s), erasing\n",
                      label, esp_err_to_name(result));
        if (nvs_flash_erase_partition(label) == ESP_OK)
            result = nvs_flash_init_partition(label);
    }
    ready = result == ESP_OK;
    if (!ready)
        Serial.printf("[CONFIG] NVS partition %s init failed: %s\n", label,
                      esp_err_to_name(result));
    return ready;
}

std::size_t NvsConfigBackend::storedSize() const {
    Preferences preferences;
    if (!ensureNvsPartition(kPartition) || !preferences.begin(kNamespace, true, kPartition)) {
        Serial.println("[CONFIG] NVS open failed: size");
        return 0U;
    }
    const std::size_t size = preferences.getBytesLength(kBlobKey);
    preferences.end();
    return size;
}

bool NvsConfigBackend::read(void* data, std::size_t size) {
    Preferences preferences;
    if (!ensureNvsPartition(kPartition) || !preferences.begin(kNamespace, true, kPartition)) {
        Serial.println("[CONFIG] NVS open failed: read");
        return false;
    }
    const std::size_t stored_size = preferences.getBytesLength(kBlobKey);
    const std::size_t read_size =
        stored_size == size ? preferences.getBytes(kBlobKey, data, size) : 0U;
    const bool read_ok = stored_size == size && read_size == size;
    preferences.end();
    if (!read_ok) {
        Serial.printf("[CONFIG] NVS read failed: expected=%u stored=%u read=%u\n",
                      static_cast<unsigned>(size),
                      static_cast<unsigned>(stored_size),
                      static_cast<unsigned>(read_size));
    }
    return read_ok;
}

bool NvsConfigBackend::write(const void* data, std::size_t size) {
    Preferences preferences;
    if (!ensureNvsPartition(kPartition) || !preferences.begin(kNamespace, false, kPartition)) {
        Serial.println("[CONFIG] NVS open failed: write");
        return false;
    }
    const std::size_t written = preferences.putBytes(kBlobKey, data, size);
    const bool write_ok = written == size;
    preferences.end();
    if (!write_ok) {
        Serial.printf("[CONFIG] NVS write failed: expected=%u actual=%u\n",
                      static_cast<unsigned>(size),
                      static_cast<unsigned>(written));
    }
    return write_ok;
}

bool NvsConfigBackend::erase() {
    Preferences preferences;
    if (!ensureNvsPartition(kPartition) || !preferences.begin(kNamespace, false, kPartition)) {
        Serial.println("[CONFIG] NVS open failed: erase");
        return false;
    }
    const bool erase_ok = preferences.clear();
    preferences.end();
    if (!erase_ok) {
        Serial.println("[CONFIG] NVS erase failed");
    }
    return erase_ok;
}

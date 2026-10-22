#include "nvs_odometer_store.h"

#include <Arduino.h>
#include <Preferences.h>

#include "settings/nvs_config_backend.h"

OdometerLoadResult NvsOdometerStore::load(OdometerRecord& record) {
    if (!ensureNvsPartition(NvsConfigBackend::kPartition)) {
        return OdometerLoadResult::Unavailable;
    }
    Preferences preferences;
    // Read-only open fails with NOT_FOUND when the namespace was never
    // written: that is a first boot, not a storage fault.
    if (!preferences.begin(kNamespace, true, NvsConfigBackend::kPartition)) {
        Preferences probe;
        if (!probe.begin(kNamespace, false, NvsConfigBackend::kPartition)) {
            return OdometerLoadResult::Unavailable;
        }
        probe.end();
        return OdometerLoadResult::Empty;
    }
    const std::size_t stored = preferences.getBytesLength(kKey);
    OdometerRecord candidate{};
    const bool read_ok =
        stored == sizeof(candidate) &&
        preferences.getBytes(kKey, &candidate, sizeof(candidate)) ==
            sizeof(candidate);
    preferences.end();
    if (stored == 0U) return OdometerLoadResult::Empty;
    if (!read_ok || !odometerRecordValid(candidate)) {
        Serial.println("[ODO] stored record rejected (size/CRC/version)");
        return OdometerLoadResult::Corrupt;
    }
    record = candidate;
    return OdometerLoadResult::Loaded;
}

bool NvsOdometerStore::save(const OdometerRecord& record) {
    Preferences preferences;
    if (!ensureNvsPartition(NvsConfigBackend::kPartition) ||
        !preferences.begin(kNamespace, false, NvsConfigBackend::kPartition)) {
        Serial.println("[ODO] NVS open failed");
        return false;
    }
    const bool ok = preferences.putBytes(kKey, &record, sizeof(record)) ==
                    sizeof(record);
    preferences.end();
    if (!ok) Serial.println("[ODO] NVS write failed");
    return ok;
}

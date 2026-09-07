#include "nvs_config_backend.h"

#include <Preferences.h>

std::size_t NvsConfigBackend::storedSize() const {
    Preferences preferences;
    if (!preferences.begin(kNamespace, true)) {
        return 0U;
    }
    const std::size_t size = preferences.getBytesLength(kBlobKey);
    preferences.end();
    return size;
}

bool NvsConfigBackend::read(void* data, std::size_t size) {
    Preferences preferences;
    if (!preferences.begin(kNamespace, true)) {
        return false;
    }
    const bool valid_size = preferences.getBytesLength(kBlobKey) == size;
    const bool read_ok =
        valid_size && preferences.getBytes(kBlobKey, data, size) == size;
    preferences.end();
    return read_ok;
}

bool NvsConfigBackend::write(const void* data, std::size_t size) {
    Preferences preferences;
    if (!preferences.begin(kNamespace, false)) {
        return false;
    }
    const bool write_ok = preferences.putBytes(kBlobKey, data, size) == size;
    preferences.end();
    return write_ok;
}

bool NvsConfigBackend::erase() {
    Preferences preferences;
    if (!preferences.begin(kNamespace, false)) {
        return false;
    }
    const bool erase_ok = preferences.clear();
    preferences.end();
    return erase_ok;
}

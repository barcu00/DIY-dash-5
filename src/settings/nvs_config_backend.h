#pragma once

#include "settings/config_repository.h"

// Initializes an NVS data partition, erasing it once if the IDF reports it
// unusable (NO_FREE_PAGES / NEW_VERSION_FOUND).
bool ensureNvsPartition(const char* label);

class NvsConfigBackend : public ConfigBackend {
public:
    std::size_t storedSize() const override;
    bool read(void* data, std::size_t size) override;
    bool write(const void* data, std::size_t size) override;
    bool erase() override;

    static constexpr const char* kPartition = "dashcfg";
    static constexpr const char* kNamespace = "diy_dash";
    static constexpr const char* kBlobKey = "config";
};

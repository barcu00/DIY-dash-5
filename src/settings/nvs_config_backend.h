#pragma once

#include "settings/config_repository.h"

class NvsConfigBackend : public ConfigBackend {
public:
    std::size_t storedSize() const override;
    bool read(void* data, std::size_t size) override;
    bool write(const void* data, std::size_t size) override;
    bool erase() override;

    static constexpr const char* kNamespace = "diy_dash";
    static constexpr const char* kBlobKey = "config";
};

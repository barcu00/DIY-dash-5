#pragma once

#include <cstddef>
#include <cstdint>

#include "settings/app_config.h"

class ConfigBackend {
public:
    virtual ~ConfigBackend() = default;
    virtual bool read(void* data, std::size_t size) = 0;
    virtual bool write(const void* data, std::size_t size) = 0;
    virtual bool erase() = 0;
};

enum class LoadResult : uint8_t {
    Loaded,
    DefaultsUsed,
};

class ConfigRepository {
public:
    explicit ConfigRepository(ConfigBackend& backend);

    LoadResult load(AppConfig& config);
    bool saveCandidate(const AppConfig& candidate, AppConfig& runtime_config);
    bool resetLayout(PageId page, AppConfig& runtime_config);
    bool reset(AppConfig& runtime_config);

private:
    ConfigBackend& backend_;
};

#pragma once

#include "settings/app_config.h"

class NvsConfigStore {
public:
    bool load(AppConfig& config);
    bool save(const AppConfig& config);
    bool clear();

    static constexpr const char* kNamespace = "opendash";
    static constexpr const char* kBlobKey = "config";
};

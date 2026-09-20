#pragma once

#include <cstdint>

#include "settings/app_config.h"

enum class ConfigCommitKind : uint8_t {
    Save,
    FactoryReset,
};

struct ConfigCommitRequest {
    ConfigCommitKind kind = ConfigCommitKind::Save;
    AppConfig candidate = AppConfig::defaults();
    bool reconfigure_runtime = false;
    uint32_t revision = 0U;
};

class SettingsCommitModel {
public:
    void markDirty(bool reconfigure_runtime);
    bool dirty() const;
    bool busy() const;
    bool queueOnExit(const AppConfig& candidate);
    bool queueFactoryReset();
    bool take(ConfigCommitRequest& request);
    bool complete(uint32_t revision, bool success);

private:
    bool dirty_ = false;
    bool pending_ = false;
    bool in_flight_ = false;
    bool reconfigure_runtime_ = false;
    uint32_t revision_ = 0U;
    uint32_t in_flight_revision_ = 0U;
    ConfigCommitRequest pending_request_{};
};

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
    bool queueOnExit(const AppConfig& candidate);
    void queueFactoryReset();
    bool take(ConfigCommitRequest& request);
    void complete(uint32_t revision, bool success);

private:
    bool dirty_ = false;
    bool pending_ = false;
    bool reconfigure_runtime_ = false;
    uint32_t revision_ = 0U;
    ConfigCommitRequest pending_request_{};
};

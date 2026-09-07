#include "settings_commit_model.h"

void SettingsCommitModel::markDirty(bool reconfigure_runtime) {
    ++revision_;
    dirty_ = true;
    reconfigure_runtime_ = reconfigure_runtime_ || reconfigure_runtime;
}

bool SettingsCommitModel::dirty() const {
    return dirty_;
}

bool SettingsCommitModel::queueOnExit(const AppConfig& candidate) {
    if (!dirty_) {
        return false;
    }

    pending_request_.kind = ConfigCommitKind::Save;
    pending_request_.candidate = candidate;
    pending_request_.reconfigure_runtime = reconfigure_runtime_;
    pending_request_.revision = revision_;
    pending_ = true;
    return true;
}

void SettingsCommitModel::queueFactoryReset() {
    ++revision_;
    dirty_ = true;
    reconfigure_runtime_ = true;
    pending_request_.kind = ConfigCommitKind::FactoryReset;
    pending_request_.candidate = AppConfig::defaults();
    pending_request_.reconfigure_runtime = true;
    pending_request_.revision = revision_;
    pending_ = true;
}

bool SettingsCommitModel::take(ConfigCommitRequest& request) {
    if (!pending_) {
        return false;
    }
    request = pending_request_;
    pending_ = false;
    return true;
}

void SettingsCommitModel::complete(uint32_t revision, bool success) {
    if (!success || revision != revision_) {
        return;
    }
    dirty_ = false;
    reconfigure_runtime_ = false;
}

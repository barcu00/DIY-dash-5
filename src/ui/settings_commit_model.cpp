#include "settings_commit_model.h"

void SettingsCommitModel::markDirty(bool reconfigure_runtime) {
    ++revision_;
    dirty_ = true;
    reconfigure_runtime_ = reconfigure_runtime_ || reconfigure_runtime;
}

bool SettingsCommitModel::dirty() const {
    return dirty_;
}

bool SettingsCommitModel::busy() const {
    return pending_ || in_flight_;
}

bool SettingsCommitModel::queueOnExit(const AppConfig& candidate) {
    if (!dirty_) {
        return false;
    }

    if (in_flight_ && in_flight_revision_ == revision_) {
        return true;
    }

    pending_request_.kind = ConfigCommitKind::Save;
    pending_request_.candidate = candidate;
    pending_request_.reconfigure_runtime = reconfigure_runtime_;
    pending_request_.revision = revision_;
    pending_ = true;
    return true;
}

bool SettingsCommitModel::queueFactoryReset() {
    if (pending_ || in_flight_) {
        return false;
    }
    ++revision_;
    dirty_ = true;
    reconfigure_runtime_ = true;
    pending_request_.kind = ConfigCommitKind::FactoryReset;
    pending_request_.candidate = AppConfig::defaults();
    pending_request_.reconfigure_runtime = true;
    pending_request_.revision = revision_;
    pending_ = true;
    return true;
}

bool SettingsCommitModel::take(ConfigCommitRequest& request) {
    if (!pending_ || in_flight_) {
        return false;
    }
    request = pending_request_;
    pending_ = false;
    in_flight_ = true;
    in_flight_revision_ = request.revision;
    return true;
}

void SettingsCommitModel::complete(uint32_t revision, bool success) {
    if (!in_flight_ || revision != in_flight_revision_) {
        return;
    }
    in_flight_ = false;
    in_flight_revision_ = 0U;
    if (success && revision == revision_) {
        dirty_ = false;
        reconfigure_runtime_ = false;
    }
}

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
    AppConfig::setDefaults(pending_request_.candidate);
    pending_request_.reconfigure_runtime = true;
    pending_request_.revision = revision_;
    pending_ = true;
    return true;
}

const ConfigCommitRequest* SettingsCommitModel::takeInPlace() {
    if (!pending_ || in_flight_) {
        return nullptr;
    }
    pending_ = false;
    in_flight_ = true;
    in_flight_revision_ = pending_request_.revision;
    return &pending_request_;
}

bool SettingsCommitModel::take(ConfigCommitRequest& request) {
    const ConfigCommitRequest* taken = takeInPlace();
    if (taken == nullptr) {
        return false;
    }
    request = *taken;
    return true;
}

bool SettingsCommitModel::complete(uint32_t revision, bool success) {
    if (!in_flight_ || revision != in_flight_revision_) {
        return false;
    }
    in_flight_ = false;
    in_flight_revision_ = 0U;
    if (success && revision == revision_) {
        dirty_ = false;
        reconfigure_runtime_ = false;
    }
    return true;
}

bool SettingsCommitModel::discardChanges() {
    if (busy()) return false;
    dirty_ = false;
    reconfigure_runtime_ = false;
    // Field-wise: a ConfigCommitRequest{} temporary is ~12 KB of stack.
    pending_request_.kind = ConfigCommitKind::Save;
    pending_request_.reconfigure_runtime = false;
    pending_request_.revision = 0U;
    return true;
}

#include "ui_update_policy.h"

void UiUpdatePolicy::activate(PageId page) {
    activate(page == PageId::Dash ? UiActivity::Dash :
             (page == PageId::Track ? UiActivity::Track
                                    : UiActivity::Settings));
}

void UiUpdatePolicy::activate(UiActivity activity) {
    activity_ = activity;
    interaction_active_ = false;
    if (activity == UiActivity::Settings) {
        settings_status_initialized_ = false;
    }
}

bool UiUpdatePolicy::shouldUpdateData(PageId page) const {
    return (page == PageId::Dash && activity_ == UiActivity::Dash) ||
           (page == PageId::Track && activity_ == UiActivity::Track);
}

void UiUpdatePolicy::markLayoutDirty() {
    layout_dirty_ = true;
}

bool UiUpdatePolicy::takeLayoutDirty() {
    const bool dirty = layout_dirty_;
    layout_dirty_ = false;
    return dirty;
}

bool UiUpdatePolicy::shouldUpdateSettingsStatus(uint32_t now_ms) {
    if (activity_ != UiActivity::Settings || interaction_active_) {
        return false;
    }
    if (!settings_status_initialized_ ||
        now_ms - last_settings_status_ms_ >= kSettingsRefreshMs) {
        settings_status_initialized_ = true;
        last_settings_status_ms_ = now_ms;
        return true;
    }
    return false;
}

void UiUpdatePolicy::setInteractionActive(bool active) {
    interaction_active_ = active;
    if (!active) {
        settings_status_initialized_ = false;
    }
}

bool UiUpdatePolicy::allowModalUpdates() const {
    return activity_ != UiActivity::TileEditor && !interaction_active_;
}

bool UiUpdatePolicy::allowShiftLightUpdates() const {
    return activity_ == UiActivity::Dash || activity_ == UiActivity::Track;
}

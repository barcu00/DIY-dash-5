#include "ui_update_policy.h"

void UiUpdatePolicy::activate(PageId page) {
    active_page_ = page;
    if (page == PageId::Settings) {
        settings_status_initialized_ = false;
    }
}

bool UiUpdatePolicy::shouldUpdateData(PageId page) const {
    return page != PageId::Settings && page == active_page_;
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
    if (active_page_ != PageId::Settings) {
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

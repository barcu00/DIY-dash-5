#pragma once

#include <cstdint>

#include "settings/app_config.h"

class UiUpdatePolicy {
public:
    void activate(PageId page);
    bool shouldUpdateData(PageId page) const;
    void markLayoutDirty();
    bool takeLayoutDirty();
    bool shouldUpdateSettingsStatus(uint32_t now_ms);
    void setInteractionActive(bool active);
    bool allowModalUpdates() const;

private:
    static constexpr uint32_t kSettingsRefreshMs = 250U;
    PageId active_page_ = PageId::Dash;
    bool layout_dirty_ = true;
    bool settings_status_initialized_ = false;
    uint32_t last_settings_status_ms_ = 0U;
    bool interaction_active_ = false;
};

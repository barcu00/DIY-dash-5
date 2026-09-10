#pragma once

#include <cstdint>

#include "settings/app_config.h"

enum class UiActivity : uint8_t {
    Dash,
    Track,
    Settings,
    TileEditor,
};

class UiUpdatePolicy {
public:
    void activate(PageId page);
    void activate(UiActivity activity);
    bool shouldUpdateData(PageId page) const;
    void markLayoutDirty();
    bool takeLayoutDirty();
    bool shouldUpdateSettingsStatus(uint32_t now_ms);
    void setInteractionActive(bool active);
    bool allowModalUpdates() const;
    bool allowLayoutUpdates() const;
    bool allowShiftLightUpdates() const;

private:
    static constexpr uint32_t kSettingsRefreshMs = 250U;
    UiActivity activity_ = UiActivity::Dash;
    bool layout_dirty_ = true;
    bool settings_status_initialized_ = false;
    uint32_t last_settings_status_ms_ = 0U;
    bool interaction_active_ = false;
};

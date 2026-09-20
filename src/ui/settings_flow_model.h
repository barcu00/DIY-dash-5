#pragma once

#include <cstddef>
#include <cstdint>

#include "settings/app_config.h"

enum class SettingsCategory : uint8_t {
    Home,
    Display,
    DataCan,
    RaceChrono,
    ShiftLight,
    Units,
    Layouts,
    System,
};

enum class SettingsInputKind : uint8_t {
    Discrete,
    Slider,
};

enum class SettingsInputEvent : uint8_t {
    ValueChanged,
    Released,
    PressLost,
};

enum class SettingsResetTarget : uint8_t {
    DashLayout,
    TrackLayout,
    Factory,
};

enum class ShiftField : uint8_t {
    Start,
    Red,
    Flash,
    Maximum,
};

class SettingsFlowModel {
public:
    static constexpr std::size_t kSlotsPerPage = 6U;

    void open(SettingsCategory category);
    void backToHome();
    SettingsCategory category() const;

    void selectLayout(PageId page);
    void setLayoutTileCount(std::size_t count);
    PageId layout() const;
    std::size_t pageIndex() const;
    std::size_t pageCount() const;
    std::size_t firstSlot() const;
    bool nextPage();
    bool previousPage();

    static bool shouldPersist(SettingsInputKind kind,
                              SettingsInputEvent event);
    static ShiftLightConfig correctedShift(ShiftLightConfig current,
                                           ShiftField field,
                                           uint16_t requested_rpm);
    void requestReset(SettingsResetTarget target);
    bool resetPending() const;
    SettingsResetTarget pendingReset() const;
    void cancelReset();

private:
    SettingsCategory category_ = SettingsCategory::Home;
    PageId layout_ = PageId::Dash;
    std::size_t page_index_ = 0U;
    std::size_t layout_tile_count_ = 14U;
    SettingsResetTarget reset_target_ = SettingsResetTarget::DashLayout;
    bool reset_pending_ = false;
};

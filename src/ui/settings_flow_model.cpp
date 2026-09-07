#include "settings_flow_model.h"

#include <algorithm>

namespace {
constexpr uint16_t kMinimumRpm = 1000U;
constexpr uint16_t kMaximumRpm = 15000U;
constexpr uint16_t kRpmGap = 100U;
}

void SettingsFlowModel::open(SettingsCategory category) {
    category_ = category;
}

void SettingsFlowModel::backToHome() {
    category_ = SettingsCategory::Home;
}

SettingsCategory SettingsFlowModel::category() const {
    return category_;
}

void SettingsFlowModel::selectLayout(PageId page) {
    if (page != PageId::Dash && page != PageId::Track) {
        return;
    }
    layout_ = page;
    page_index_ = 0U;
}

PageId SettingsFlowModel::layout() const {
    return layout_;
}

std::size_t SettingsFlowModel::pageIndex() const {
    return page_index_;
}

std::size_t SettingsFlowModel::pageCount() const {
    const std::size_t slots = layout_ == PageId::Dash
                                  ? AppConfig::kDashTileCount
                                  : AppConfig::kTrackTileCount;
    return (slots + kSlotsPerPage - 1U) / kSlotsPerPage;
}

std::size_t SettingsFlowModel::firstSlot() const {
    return page_index_ * kSlotsPerPage;
}

bool SettingsFlowModel::nextPage() {
    if (page_index_ + 1U >= pageCount()) {
        return false;
    }
    ++page_index_;
    return true;
}

bool SettingsFlowModel::previousPage() {
    if (page_index_ == 0U) {
        return false;
    }
    --page_index_;
    return true;
}

bool SettingsFlowModel::shouldPersist(SettingsInputKind kind,
                                      SettingsInputEvent event) {
    return kind == SettingsInputKind::Discrete
               ? event == SettingsInputEvent::ValueChanged
               : event == SettingsInputEvent::Released;
}

ShiftLightConfig SettingsFlowModel::correctedShift(
    ShiftLightConfig current, ShiftField field, uint16_t requested_rpm) {
    if (field == ShiftField::Start) {
        current.start_rpm = std::clamp<uint16_t>(
            requested_rpm, kMinimumRpm, kMaximumRpm - 2U * kRpmGap);
        current.red_rpm = std::clamp<uint16_t>(
            std::max<uint16_t>(current.red_rpm,
                               current.start_rpm + kRpmGap),
            current.start_rpm + kRpmGap, kMaximumRpm - kRpmGap);
        current.max_rpm = std::clamp<uint16_t>(
            std::max<uint16_t>(current.max_rpm,
                               current.red_rpm + kRpmGap),
            current.red_rpm + kRpmGap, kMaximumRpm);
        return current;
    }

    if (field == ShiftField::Red) {
        current.red_rpm = std::clamp<uint16_t>(
            requested_rpm, kMinimumRpm + kRpmGap,
            kMaximumRpm - kRpmGap);
        current.start_rpm = std::clamp<uint16_t>(
            std::min<uint16_t>(current.start_rpm,
                               current.red_rpm - kRpmGap),
            kMinimumRpm, current.red_rpm - kRpmGap);
        current.max_rpm = std::clamp<uint16_t>(
            std::max<uint16_t>(current.max_rpm,
                               current.red_rpm + kRpmGap),
            current.red_rpm + kRpmGap, kMaximumRpm);
        return current;
    }

    current.max_rpm = std::clamp<uint16_t>(
        requested_rpm, kMinimumRpm + 2U * kRpmGap, kMaximumRpm);
    current.red_rpm = std::clamp<uint16_t>(
        std::min<uint16_t>(current.red_rpm,
                           current.max_rpm - kRpmGap),
        kMinimumRpm + kRpmGap, current.max_rpm - kRpmGap);
    current.start_rpm = std::clamp<uint16_t>(
        std::min<uint16_t>(current.start_rpm,
                           current.red_rpm - kRpmGap),
        kMinimumRpm, current.red_rpm - kRpmGap);
    return current;
}

void SettingsFlowModel::requestReset(SettingsResetTarget target) {
    reset_target_ = target;
    reset_pending_ = true;
}

bool SettingsFlowModel::resetPending() const {
    return reset_pending_;
}

SettingsResetTarget SettingsFlowModel::pendingReset() const {
    return reset_target_;
}

void SettingsFlowModel::cancelReset() {
    reset_pending_ = false;
}

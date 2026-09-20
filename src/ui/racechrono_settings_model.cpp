#include "racechrono_settings_model.h"

#include <algorithm>

void RaceChronoSettingsModel::setChannelCount(std::size_t count) {
    channel_count_ = std::min(count, kMaximumChannels);
    clampPage();
}

void RaceChronoSettingsModel::setChannelState(
    std::size_t catalog_index, RaceChronoChannelState state) {
    if (catalog_index >= channel_count_) return;
    states_[catalog_index] = state;
    clampPage();
}

void RaceChronoSettingsModel::setFilter(RaceChronoChannelFilter filter) {
    filter_ = filter;
    page_index_ = 0U;
}

RaceChronoChannelFilter RaceChronoSettingsModel::filter() const {
    return filter_;
}

bool RaceChronoSettingsModel::nextPage() {
    if (page_index_ + 1U >= pageCount()) return false;
    ++page_index_;
    return true;
}

bool RaceChronoSettingsModel::previousPage() {
    if (page_index_ == 0U) return false;
    --page_index_;
    return true;
}

std::size_t RaceChronoSettingsModel::pageIndex() const {
    return page_index_;
}

std::size_t RaceChronoSettingsModel::pageCount() const {
    const std::size_t count = filteredCount();
    return std::max<std::size_t>(1U,
        (count + kRowsPerPage - 1U) / kRowsPerPage);
}

std::size_t RaceChronoSettingsModel::firstIndex() const {
    return page_index_ * kRowsPerPage;
}

std::size_t RaceChronoSettingsModel::rowsOnPage() const {
    const std::size_t count = filteredCount();
    const std::size_t first = firstIndex();
    if (first >= count) return 0U;
    return std::min(kRowsPerPage, count - first);
}

std::size_t RaceChronoSettingsModel::catalogIndexAtRow(std::size_t row) const {
    if (row >= rowsOnPage()) return channel_count_;
    return filteredCatalogIndex(firstIndex() + row);
}

bool RaceChronoSettingsModel::matches(std::size_t catalog_index) const {
    if (filter_ == RaceChronoChannelFilter::All) return true;
    const RaceChronoChannelState state = states_[catalog_index];
    if (filter_ == RaceChronoChannelFilter::Active)
        return state == RaceChronoChannelState::Active;
    if (filter_ == RaceChronoChannelFilter::NoData)
        return state == RaceChronoChannelState::NoData;
    return state == RaceChronoChannelState::Error;
}

std::size_t RaceChronoSettingsModel::filteredCount() const {
    std::size_t count = 0U;
    for (std::size_t index = 0U; index < channel_count_; ++index)
        if (matches(index)) ++count;
    return count;
}

std::size_t RaceChronoSettingsModel::filteredCatalogIndex(
    std::size_t filtered_index) const {
    std::size_t match = 0U;
    for (std::size_t index = 0U; index < channel_count_; ++index) {
        if (!matches(index)) continue;
        if (match == filtered_index) return index;
        ++match;
    }
    return channel_count_;
}

void RaceChronoSettingsModel::clampPage() {
    const std::size_t pages = pageCount();
    if (page_index_ >= pages) page_index_ = pages - 1U;
}

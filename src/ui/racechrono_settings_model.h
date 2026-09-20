#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "racechrono/racechrono_telemetry.h"

enum class RaceChronoChannelFilter : uint8_t {
    All,
    Active,
    NoData,
    Error,
};

class RaceChronoSettingsModel {
public:
    static constexpr std::size_t kRowsPerPage = 6U;
    static constexpr std::size_t kMaximumChannels = 33U;

    void setChannelCount(std::size_t count);
    void setChannelState(std::size_t catalog_index,
                         RaceChronoChannelState state);
    void setFilter(RaceChronoChannelFilter filter);
    RaceChronoChannelFilter filter() const;

    bool nextPage();
    bool previousPage();
    std::size_t pageIndex() const;
    std::size_t pageCount() const;
    std::size_t firstIndex() const;
    std::size_t rowsOnPage() const;
    std::size_t catalogIndexAtRow(std::size_t row) const;

private:
    bool matches(std::size_t catalog_index) const;
    std::size_t filteredCount() const;
    std::size_t filteredCatalogIndex(std::size_t filtered_index) const;
    void clampPage();

    std::array<RaceChronoChannelState, kMaximumChannels> states_{};
    std::size_t channel_count_ = 0U;
    std::size_t page_index_ = 0U;
    RaceChronoChannelFilter filter_ = RaceChronoChannelFilter::All;
};

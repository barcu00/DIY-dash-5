#include <unity.h>

#include "ui/racechrono_settings_model.h"

namespace {

void setAlternatingStates(RaceChronoSettingsModel& model) {
    model.setChannelCount(33U);
    for (std::size_t index = 0U; index < 33U; ++index) {
        const RaceChronoChannelState state =
            index % 3U == 0U ? RaceChronoChannelState::Active
            : index % 3U == 1U ? RaceChronoChannelState::NoData
                               : RaceChronoChannelState::Error;
        model.setChannelState(index, state);
    }
}

}  // namespace

void test_all_filter_pages_33_channels_in_six_fixed_rows() {
    RaceChronoSettingsModel model;
    model.setChannelCount(33U);

    TEST_ASSERT_EQUAL_UINT32(6U, model.pageCount());
    constexpr std::size_t expected_first[] = {0U, 6U, 12U, 18U, 24U, 30U};
    for (std::size_t page = 0U; page < 6U; ++page) {
        TEST_ASSERT_EQUAL_UINT32(expected_first[page], model.firstIndex());
        TEST_ASSERT_EQUAL_UINT32(page == 5U ? 3U : 6U, model.rowsOnPage());
        if (page < 5U) TEST_ASSERT_TRUE(model.nextPage());
    }
    TEST_ASSERT_FALSE(model.nextPage());
    TEST_ASSERT_EQUAL_UINT32(30U, model.firstIndex());
}

void test_filters_preserve_catalog_order_and_reset_to_first_page() {
    RaceChronoSettingsModel model;
    setAlternatingStates(model);
    TEST_ASSERT_TRUE(model.nextPage());

    model.setFilter(RaceChronoChannelFilter::Active);
    TEST_ASSERT_EQUAL_UINT32(0U, model.firstIndex());
    TEST_ASSERT_EQUAL_UINT32(2U, model.pageCount());
    TEST_ASSERT_EQUAL_UINT32(0U, model.catalogIndexAtRow(0U));
    TEST_ASSERT_EQUAL_UINT32(3U, model.catalogIndexAtRow(1U));
    TEST_ASSERT_EQUAL_UINT32(6U, model.catalogIndexAtRow(2U));

    model.setFilter(RaceChronoChannelFilter::NoData);
    TEST_ASSERT_EQUAL_UINT32(0U, model.firstIndex());
    TEST_ASSERT_EQUAL_UINT32(1U, model.catalogIndexAtRow(0U));
    TEST_ASSERT_EQUAL_UINT32(4U, model.catalogIndexAtRow(1U));

    model.setFilter(RaceChronoChannelFilter::Error);
    TEST_ASSERT_EQUAL_UINT32(0U, model.firstIndex());
    TEST_ASSERT_EQUAL_UINT32(2U, model.catalogIndexAtRow(0U));
    TEST_ASSERT_EQUAL_UINT32(5U, model.catalogIndexAtRow(1U));

    model.setFilter(RaceChronoChannelFilter::All);
    TEST_ASSERT_EQUAL_UINT32(0U, model.firstIndex());
    TEST_ASSERT_EQUAL_UINT32(0U, model.catalogIndexAtRow(0U));
    TEST_ASSERT_EQUAL_UINT32(1U, model.catalogIndexAtRow(1U));
}

void test_previous_and_next_page_clamp_at_both_ends() {
    RaceChronoSettingsModel model;
    model.setChannelCount(13U);

    TEST_ASSERT_FALSE(model.previousPage());
    TEST_ASSERT_TRUE(model.nextPage());
    TEST_ASSERT_TRUE(model.nextPage());
    TEST_ASSERT_FALSE(model.nextPage());
    TEST_ASSERT_EQUAL_UINT32(12U, model.firstIndex());
    TEST_ASSERT_EQUAL_UINT32(1U, model.rowsOnPage());
    TEST_ASSERT_TRUE(model.previousPage());
    TEST_ASSERT_TRUE(model.previousPage());
    TEST_ASSERT_FALSE(model.previousPage());
    TEST_ASSERT_EQUAL_UINT32(0U, model.firstIndex());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_all_filter_pages_33_channels_in_six_fixed_rows);
    RUN_TEST(test_filters_preserve_catalog_order_and_reset_to_first_page);
    RUN_TEST(test_previous_and_next_page_clamp_at_both_ends);
    return UNITY_END();
}

#include <unity.h>

#include "ui/icon_assets.h"
#include "ui/icon_catalog.h"

namespace {
uint16_t litPixels(const IconAsset& asset) {
    uint16_t total = 0U;
    if (asset.rows == nullptr) return total;
    for (uint8_t y = 0; y < asset.height; ++y) {
        uint16_t row = asset.rows[y];
        for (uint8_t x = 0; x < asset.width; ++x) {
            if ((row & static_cast<uint16_t>(1U << (15U - x))) != 0U) ++total;
        }
    }
    return total;
}

void test_every_catalog_icon_has_a_visual_asset() {
    for (uint16_t raw = static_cast<uint16_t>(IconId::Generic);
         raw < static_cast<uint16_t>(IconId::Count); ++raw) {
        const auto id = static_cast<IconId>(raw);
        const IconAsset& asset = IconAssets::get(id);
        TEST_ASSERT_NOT_NULL(asset.rows);
        TEST_ASSERT_EQUAL_UINT8(16U, asset.width);
        TEST_ASSERT_EQUAL_UINT8(16U, asset.height);
        TEST_ASSERT_EQUAL_UINT16(16U, asset.row_count);
        TEST_ASSERT_GREATER_THAN_UINT16(8U, litPixels(asset));
    }
}

void test_invalid_icon_has_no_visual_asset() {
    const IconAsset& asset = IconAssets::get(IconId::Invalid);
    TEST_ASSERT_NULL(asset.rows);
    TEST_ASSERT_EQUAL_UINT8(0U, asset.width);
    TEST_ASSERT_EQUAL_UINT8(0U, asset.height);
}

void test_core_icons_have_distinct_masks() {
    const IconAsset& rpm = IconAssets::get(IconId::Rpm);
    const IconAsset& boost = IconAssets::get(IconId::Boost);
    const IconAsset& lambda = IconAssets::get(IconId::Lambda);
    const IconAsset& oil = IconAssets::get(IconId::OilPressure);
    TEST_ASSERT_NOT_EQUAL(rpm.rows, boost.rows);
    TEST_ASSERT_NOT_EQUAL(boost.rows, lambda.rows);
    TEST_ASSERT_NOT_EQUAL(rpm.rows, lambda.rows);
    TEST_ASSERT_NOT_EQUAL(oil.rows, rpm.rows);
}

void test_simple_icons_fit_tile_readability_budget() {
    const IconId sample[] = {
        IconId::Rpm, IconId::Boost, IconId::Temperature, IconId::OilPressure,
        IconId::Lambda, IconId::Battery, IconId::Egt, IconId::Warning
    };
    for (IconId id : sample) {
        const uint16_t pixels = litPixels(IconAssets::get(id));
        TEST_ASSERT_GREATER_THAN_UINT16(12U, pixels);
        TEST_ASSERT_LESS_THAN_UINT16(150U, pixels);
    }
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_every_catalog_icon_has_a_visual_asset);
    RUN_TEST(test_invalid_icon_has_no_visual_asset);
    RUN_TEST(test_core_icons_have_distinct_masks);
    RUN_TEST(test_simple_icons_fit_tile_readability_budget);
    return UNITY_END();
}

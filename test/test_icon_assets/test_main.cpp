#include <unity.h>

#include "ui/icon_assets.h"
#include "ui/icon_catalog.h"

namespace {
void test_every_catalog_icon_has_a_visual_asset() {
    for (uint16_t raw = static_cast<uint16_t>(IconId::Generic);
         raw <= static_cast<uint16_t>(IconId::LapTime); ++raw) {
        const auto id = static_cast<IconId>(raw);
        const IconAsset& asset = IconAssets::get(id);
        TEST_ASSERT_NOT_NULL(asset.rows);
        TEST_ASSERT_EQUAL_UINT8(16U, asset.width);
        TEST_ASSERT_EQUAL_UINT8(16U, asset.height);
        TEST_ASSERT_GREATER_THAN_UINT16(0U, asset.row_count);
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
    TEST_ASSERT_NOT_EQUAL(rpm.rows, boost.rows);
    TEST_ASSERT_NOT_EQUAL(boost.rows, lambda.rows);
    TEST_ASSERT_NOT_EQUAL(rpm.rows, lambda.rows);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_every_catalog_icon_has_a_visual_asset);
    RUN_TEST(test_invalid_icon_has_no_visual_asset);
    RUN_TEST(test_core_icons_have_distinct_masks);
    return UNITY_END();
}

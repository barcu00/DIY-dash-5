#include <unity.h>

#include "ui/ui_update_policy.h"

void test_only_active_page_receives_live_data_updates() {
    UiUpdatePolicy policy;

    policy.activate(PageId::Settings);
    TEST_ASSERT_FALSE(policy.shouldUpdateData(PageId::Dash));
    TEST_ASSERT_FALSE(policy.shouldUpdateData(PageId::Track));

    policy.activate(PageId::Track);
    TEST_ASSERT_FALSE(policy.shouldUpdateData(PageId::Dash));
    TEST_ASSERT_TRUE(policy.shouldUpdateData(PageId::Track));
}

void test_layout_is_recomputed_once_per_configuration_change() {
    UiUpdatePolicy policy;

    TEST_ASSERT_TRUE(policy.takeLayoutDirty());
    TEST_ASSERT_FALSE(policy.takeLayoutDirty());

    policy.markLayoutDirty();
    TEST_ASSERT_TRUE(policy.takeLayoutDirty());
    TEST_ASSERT_FALSE(policy.takeLayoutDirty());
}

void test_settings_status_is_throttled_between_interactions() {
    UiUpdatePolicy policy;
    policy.activate(PageId::Settings);

    TEST_ASSERT_TRUE(policy.shouldUpdateSettingsStatus(1000U));
    TEST_ASSERT_FALSE(policy.shouldUpdateSettingsStatus(1050U));
    TEST_ASSERT_FALSE(policy.shouldUpdateSettingsStatus(1249U));
    TEST_ASSERT_TRUE(policy.shouldUpdateSettingsStatus(1250U));
}

void test_control_interaction_suspends_nonessential_visual_updates() {
    UiUpdatePolicy policy;
    policy.activate(PageId::Settings);
    policy.setInteractionActive(true);

    TEST_ASSERT_FALSE(policy.shouldUpdateSettingsStatus(1000U));
    TEST_ASSERT_FALSE(policy.allowModalUpdates());

    policy.setInteractionActive(false);
    TEST_ASSERT_TRUE(policy.shouldUpdateSettingsStatus(1001U));
    TEST_ASSERT_TRUE(policy.allowModalUpdates());
}

void test_tile_editor_suspends_every_background_visual_update() {
    UiUpdatePolicy policy;
    policy.activate(PageId::Dash);
    TEST_ASSERT_TRUE(policy.shouldUpdateData(PageId::Dash));
    TEST_ASSERT_TRUE(policy.allowShiftLightUpdates());

    policy.activate(UiActivity::TileEditor);

    TEST_ASSERT_FALSE(policy.shouldUpdateData(PageId::Dash));
    TEST_ASSERT_FALSE(policy.shouldUpdateData(PageId::Track));
    TEST_ASSERT_FALSE(policy.shouldUpdateSettingsStatus(1000U));
    TEST_ASSERT_FALSE(policy.allowShiftLightUpdates());
    TEST_ASSERT_FALSE(policy.allowModalUpdates());
}

void test_leaving_tile_editor_restores_the_destination_page() {
    UiUpdatePolicy policy;
    policy.activate(UiActivity::TileEditor);

    policy.activate(PageId::Track);

    TEST_ASSERT_FALSE(policy.shouldUpdateData(PageId::Dash));
    TEST_ASSERT_TRUE(policy.shouldUpdateData(PageId::Track));
    TEST_ASSERT_TRUE(policy.allowShiftLightUpdates());
    TEST_ASSERT_TRUE(policy.allowModalUpdates());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_only_active_page_receives_live_data_updates);
    RUN_TEST(test_layout_is_recomputed_once_per_configuration_change);
    RUN_TEST(test_settings_status_is_throttled_between_interactions);
    RUN_TEST(test_control_interaction_suspends_nonessential_visual_updates);
    RUN_TEST(test_tile_editor_suspends_every_background_visual_update);
    RUN_TEST(test_leaving_tile_editor_restores_the_destination_page);
    return UNITY_END();
}

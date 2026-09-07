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

void test_settings_status_is_throttled_while_scrolling() {
    UiUpdatePolicy policy;
    policy.activate(PageId::Settings);

    TEST_ASSERT_TRUE(policy.shouldUpdateSettingsStatus(1000U));
    TEST_ASSERT_FALSE(policy.shouldUpdateSettingsStatus(1050U));
    TEST_ASSERT_FALSE(policy.shouldUpdateSettingsStatus(1249U));
    TEST_ASSERT_TRUE(policy.shouldUpdateSettingsStatus(1250U));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_only_active_page_receives_live_data_updates);
    RUN_TEST(test_layout_is_recomputed_once_per_configuration_change);
    RUN_TEST(test_settings_status_is_throttled_while_scrolling);
    return UNITY_END();
}

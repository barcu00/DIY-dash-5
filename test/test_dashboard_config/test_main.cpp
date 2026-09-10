#include <unity.h>

#include "config/dashboard_config.h"

void test_dashboard_render_matches_the_forty_hertz_panel_cadence() {
    TEST_ASSERT_EQUAL_UINT32(25U, DashboardConfig::kUiUpdateIntervalMs);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_dashboard_render_matches_the_forty_hertz_panel_cadence);
    return UNITY_END();
}

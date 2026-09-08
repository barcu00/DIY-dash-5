#include <unity.h>

#include "config/dashboard_config.h"

void test_dashboard_data_refresh_is_fifty_hertz() {
    TEST_ASSERT_EQUAL_UINT32(20U, DashboardConfig::kUiUpdateIntervalMs);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_dashboard_data_refresh_is_fifty_hertz);
    return UNITY_END();
}

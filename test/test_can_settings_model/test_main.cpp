#include <unity.h>

#include "ui/can_settings_model.h"

void setUp() {}
void tearDown() {}

namespace {
void test_bitrate_cycles_supported_values() {
    AppConfig cfg = AppConfig::defaults();
    cfg.can_bitrate = 125000U;
    TEST_ASSERT_EQUAL_UINT32(250000U, CanSettingsModel::nextBitrate(cfg));
    TEST_ASSERT_EQUAL_UINT32(500000U, CanSettingsModel::nextBitrate(cfg));
    TEST_ASSERT_EQUAL_UINT32(1000000U, CanSettingsModel::nextBitrate(cfg));
    TEST_ASSERT_EQUAL_UINT32(125000U, CanSettingsModel::nextBitrate(cfg));
}

void test_source_cycles_all_profiles() {
    AppConfig cfg = AppConfig::defaults();
    cfg.data_source = DataSource::Mock;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DataSource::Ecumaster), static_cast<int>(CanSettingsModel::nextSource(cfg)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DataSource::Rusefi), static_cast<int>(CanSettingsModel::nextSource(cfg)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DataSource::Mock), static_cast<int>(CanSettingsModel::nextSource(cfg)));
}

void test_base_id_adjustment_stays_standard_and_keeps_eight_frame_window() {
    AppConfig cfg = AppConfig::defaults();
    cfg.ecumaster_base_id = 0x600U;
    TEST_ASSERT_EQUAL_HEX16(0x610U, CanSettingsModel::adjustBaseId(cfg, 0x10));
    cfg.ecumaster_base_id = 0x7F8U;
    TEST_ASSERT_EQUAL_HEX16(0x7F8U, CanSettingsModel::adjustBaseId(cfg, 0x10));
    cfg.ecumaster_base_id = 0x000U;
    TEST_ASSERT_EQUAL_HEX16(0x000U, CanSettingsModel::adjustBaseId(cfg, -0x10));
}

void test_timeout_adjustment_is_bounded() {
    AppConfig cfg = AppConfig::defaults();
    cfg.can_timeout_ms = 500U;
    TEST_ASSERT_EQUAL_UINT32(600U, CanSettingsModel::adjustTimeout(cfg, 100));
    cfg.can_timeout_ms = 50U;
    TEST_ASSERT_EQUAL_UINT32(50U, CanSettingsModel::adjustTimeout(cfg, -100));
    cfg.can_timeout_ms = 10000U;
    TEST_ASSERT_EQUAL_UINT32(10000U, CanSettingsModel::adjustTimeout(cfg, 100));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_bitrate_cycles_supported_values);
    RUN_TEST(test_source_cycles_all_profiles);
    RUN_TEST(test_base_id_adjustment_stays_standard_and_keeps_eight_frame_window);
    RUN_TEST(test_timeout_adjustment_is_bounded);
    return UNITY_END();
}

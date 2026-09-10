#include <cstring>

#include <unity.h>

#include "ecu/can_profile_registry.h"

void test_registry_contains_all_source_audited_profiles() {
    constexpr const char* ids[] = {
        "ecumaster_emu_black",
        "rusefi_verbose",
        "maxxecu_default_1_3",
        "haltech_broadcast_2_0",
        "speeduino_haltech",
        "bmw_ms43_stock",
        "link_generic_dash_experimental",
        "psa_c2_vts_engine_experimental",
    };

    TEST_ASSERT_EQUAL_UINT32(8U, CanProfileRegistry::count());
    for (const char* id : ids) {
        const CanProfile* profile = CanProfileRegistry::find(id);
        TEST_ASSERT_NOT_NULL(profile);
        TEST_ASSERT_EQUAL_STRING(id, profile->id);
        TEST_ASSERT_NOT_NULL(profile->name);
        TEST_ASSERT_NOT_NULL(profile->source_url);
        TEST_ASSERT_NOT_NULL(profile->source_revision);
        TEST_ASSERT_TRUE(profile->frame_count > 0U);
    }
    TEST_ASSERT_NULL(CanProfileRegistry::find("none"));
    TEST_ASSERT_NULL(CanProfileRegistry::find("unknown-profile"));
}

void test_verified_profiles_precede_experimental_profiles() {
    bool seen_experimental = false;
    for (std::size_t i = 0U; i < CanProfileRegistry::count(); ++i) {
        const CanProfile* profile = CanProfileRegistry::at(i);
        TEST_ASSERT_NOT_NULL(profile);
        if (profile->verification == CanProfileVerification::Experimental) {
            seen_experimental = true;
        } else {
            TEST_ASSERT_FALSE(seen_experimental);
        }
    }

    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(CanProfileVerification::Experimental),
        static_cast<uint8_t>(CanProfileRegistry::find(
            "link_generic_dash_experimental")->verification));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(CanProfileVerification::Experimental),
        static_cast<uint8_t>(CanProfileRegistry::find(
            "psa_c2_vts_engine_experimental")->verification));
}

void test_profile_metadata_preserves_documented_defaults() {
    TEST_ASSERT_EQUAL_UINT32(
        1000000U,
        CanProfileRegistry::find("ecumaster_emu_black")->default_bitrate);
    TEST_ASSERT_EQUAL_UINT32(
        500000U,
        CanProfileRegistry::find("maxxecu_default_1_3")->default_bitrate);
    TEST_ASSERT_EQUAL_UINT32(
        500000U,
        CanProfileRegistry::find("psa_c2_vts_engine_experimental")
            ->default_bitrate);

    const CanProfile* ms43 = CanProfileRegistry::find("bmw_ms43_stock");
    TEST_ASSERT_NOT_NULL(ms43);
    TEST_ASSERT_EQUAL_UINT32(500000U, ms43->default_bitrate);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(CanProfileVerification::Verified),
        static_cast<uint8_t>(ms43->verification));
    TEST_ASSERT_NOT_NULL(std::strstr(ms43->source_url, "ms4x.net"));
    TEST_ASSERT_NULL(CanProfileRegistry::find("bmw_ms43_olm"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_contains_all_source_audited_profiles);
    RUN_TEST(test_verified_profiles_precede_experimental_profiles);
    RUN_TEST(test_profile_metadata_preserves_documented_defaults);
    return UNITY_END();
}

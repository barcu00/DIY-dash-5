#include <cstddef>
#include <cstring>

#include <unity.h>

#include "ui/parameter_options.h"
#include "ecu/can_profile_registry.h"

void test_tile_parameter_options_include_every_registered_parameter() {
    char options[2048]{};

    TEST_ASSERT_TRUE(ParameterOptions::write(options, sizeof(options)));
    TEST_ASSERT_EQUAL_STRING("RPM", std::strtok(options, "\n"));

    std::size_t count = 1U;
    const char* last = options;
    while (const char* option = std::strtok(nullptr, "\n")) {
        last = option;
        ++count;
    }
    TEST_ASSERT_EQUAL_UINT32(parameterCount(), count);
    TEST_ASSERT_EQUAL_STRING("CRUISE", last);
}

void test_tile_parameter_options_report_truncation() {
    char options[8]{};
    TEST_ASSERT_FALSE(ParameterOptions::write(options, sizeof(options)));
}

void test_filtered_options_map_indexes_to_stable_parameter_ids() {
    const ParameterOptionList options = ParameterOptions::build(
        DataSource::Can, CanProfileRegistry::find("bmw_ms43_stock"),
        ParameterId::Rpm);
    char text[2048]{};

    TEST_ASSERT_TRUE(options.write(text, sizeof(text)));
    TEST_ASSERT_EQUAL_UINT32(22U, options.count());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Rpm),
                            static_cast<uint8_t>(options.parameterAt(0U)));
    TEST_ASSERT_TRUE(std::strstr(text, "CHECK ENGINE") != nullptr);
    TEST_ASSERT_TRUE(std::strstr(text, "LAMBDA") == nullptr);
}

void test_unsupported_saved_parameter_is_retained_and_labeled_in_english() {
    const ParameterOptionList options = ParameterOptions::build(
        DataSource::Can, CanProfileRegistry::find("bmw_ms43_stock"),
        ParameterId::Lambda);
    char text[2048]{};

    TEST_ASSERT_TRUE(options.write(text, sizeof(text)));
    TEST_ASSERT_EQUAL_UINT32(23U, options.count());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Lambda),
                            static_cast<uint8_t>(options.parameterAt(0U)));
    TEST_ASSERT_EQUAL_STRING("LAMBDA (UNAVAILABLE)", std::strtok(text, "\n"));
    for (std::size_t index = 1U; index < options.count(); ++index) {
        TEST_ASSERT_NOT_EQUAL(
            static_cast<uint8_t>(ParameterId::Lambda),
            static_cast<uint8_t>(options.parameterAt(index)));
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_tile_parameter_options_include_every_registered_parameter);
    RUN_TEST(test_tile_parameter_options_report_truncation);
    RUN_TEST(test_filtered_options_map_indexes_to_stable_parameter_ids);
    RUN_TEST(test_unsupported_saved_parameter_is_retained_and_labeled_in_english);
    return UNITY_END();
}

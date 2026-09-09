#include <cstddef>
#include <cstring>

#include <unity.h>

#include "ui/parameter_options.h"

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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_tile_parameter_options_include_every_registered_parameter);
    RUN_TEST(test_tile_parameter_options_report_truncation);
    return UNITY_END();
}

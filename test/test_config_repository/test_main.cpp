#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <unity.h>

// Production changes caught: corrupt configuration being accepted, failed
// writes changing runtime state, or reset erasing without restoring defaults.

#include "settings/config_repository.h"

namespace {
class MemoryBackend : public ConfigBackend {
public:
    bool read(void* data, std::size_t size) override {
        if (!has_value || size != stored_size) {
            return false;
        }
        std::memcpy(data, bytes.data(), size);
        return true;
    }

    bool write(const void* data, std::size_t size) override {
        if (fail_writes || size > bytes.size()) {
            return false;
        }
        std::memcpy(bytes.data(), data, size);
        stored_size = size;
        has_value = true;
        return true;
    }

    bool erase() override {
        if (fail_erases) {
            return false;
        }
        has_value = false;
        stored_size = 0U;
        return true;
    }

    std::array<uint8_t, sizeof(AppConfig)> bytes{};
    std::size_t stored_size = 0U;
    bool has_value = false;
    bool fail_writes = false;
    bool fail_erases = false;
};
}  // namespace

void test_missing_configuration_loads_safe_defaults() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig loaded{};

    const LoadResult result = repository.load(loaded);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::DefaultsUsed),
                            static_cast<uint8_t>(result));
    TEST_ASSERT_EQUAL_UINT32(AppConfig::kSchemaVersion, loaded.schema_version);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(loaded.data_source));
}

void test_schema_mismatch_loads_safe_defaults() {
    MemoryBackend backend;
    AppConfig incompatible = AppConfig::defaults();
    incompatible.schema_version = 0U;
    TEST_ASSERT_TRUE(backend.write(&incompatible, sizeof(incompatible)));
    ConfigRepository repository(backend);
    AppConfig loaded{};

    const LoadResult result = repository.load(loaded);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::DefaultsUsed),
                            static_cast<uint8_t>(result));
    TEST_ASSERT_EQUAL_UINT32(AppConfig::kSchemaVersion, loaded.schema_version);
}

void test_valid_configuration_round_trips_through_backend() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.brightness_percent = 40U;
    candidate.dash_tiles[3].visible = false;

    TEST_ASSERT_TRUE(repository.saveCandidate(candidate, runtime));
    AppConfig reloaded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::Loaded),
                            static_cast<uint8_t>(repository.load(reloaded)));
    TEST_ASSERT_EQUAL_UINT8(40U, reloaded.brightness_percent);
    TEST_ASSERT_FALSE(reloaded.dash_tiles[3].visible);
}

void test_failed_save_keeps_previous_runtime_configuration() {
    MemoryBackend backend;
    backend.fail_writes = true;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.brightness_percent = 40U;

    TEST_ASSERT_FALSE(repository.saveCandidate(candidate, runtime));
    TEST_ASSERT_EQUAL_UINT8(100U, runtime.brightness_percent);
}

void test_invalid_candidate_is_not_written_or_applied() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.shift = ShiftLightConfig{7000U, 6000U, 5000U};

    TEST_ASSERT_FALSE(repository.saveCandidate(candidate, runtime));
    TEST_ASSERT_FALSE(backend.has_value);
    TEST_ASSERT_EQUAL_UINT16(5500U, runtime.shift.start_rpm);
}

void test_reset_erases_storage_and_restores_defaults_only_after_success() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    runtime.brightness_percent = 30U;

    TEST_ASSERT_TRUE(repository.reset(runtime));
    TEST_ASSERT_EQUAL_UINT8(100U, runtime.brightness_percent);
    TEST_ASSERT_FALSE(backend.has_value);

    runtime.brightness_percent = 30U;
    backend.fail_erases = true;
    TEST_ASSERT_FALSE(repository.reset(runtime));
    TEST_ASSERT_EQUAL_UINT8(30U, runtime.brightness_percent);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_missing_configuration_loads_safe_defaults);
    RUN_TEST(test_schema_mismatch_loads_safe_defaults);
    RUN_TEST(test_valid_configuration_round_trips_through_backend);
    RUN_TEST(test_failed_save_keeps_previous_runtime_configuration);
    RUN_TEST(test_invalid_candidate_is_not_written_or_applied);
    RUN_TEST(test_reset_erases_storage_and_restores_defaults_only_after_success);
    return UNITY_END();
}

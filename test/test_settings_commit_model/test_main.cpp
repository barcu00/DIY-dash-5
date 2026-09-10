#include <unity.h>

#include "ui/settings_commit_model.h"

void test_unchanged_exit_does_not_queue_a_commit() {
    SettingsCommitModel model;
    ConfigCommitRequest request;

    TEST_ASSERT_FALSE(model.queueOnExit(AppConfig::defaults()));
    TEST_ASSERT_FALSE(model.take(request));
    TEST_ASSERT_FALSE(model.dirty());
}

void test_dirty_exit_queues_the_current_snapshot() {
    SettingsCommitModel model;
    AppConfig candidate = AppConfig::defaults();
    candidate.brightness_percent = 60U;
    model.markDirty(false);

    TEST_ASSERT_TRUE(model.queueOnExit(candidate));
    ConfigCommitRequest request;
    TEST_ASSERT_TRUE(model.take(request));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConfigCommitKind::Save),
                            static_cast<uint8_t>(request.kind));
    TEST_ASSERT_EQUAL_UINT8(60U, request.candidate.brightness_percent);
    TEST_ASSERT_FALSE(request.reconfigure_runtime);
}

void test_repeated_edits_coalesce_to_the_latest_snapshot() {
    SettingsCommitModel model;
    AppConfig candidate = AppConfig::defaults();
    candidate.brightness_percent = 80U;
    model.markDirty(false);
    TEST_ASSERT_TRUE(model.queueOnExit(candidate));

    candidate.brightness_percent = 40U;
    model.markDirty(true);
    TEST_ASSERT_TRUE(model.queueOnExit(candidate));

    ConfigCommitRequest request;
    TEST_ASSERT_TRUE(model.take(request));
    TEST_ASSERT_EQUAL_UINT8(40U, request.candidate.brightness_percent);
    TEST_ASSERT_TRUE(request.reconfigure_runtime);
    TEST_ASSERT_FALSE(model.take(request));
}

void test_success_cleans_matching_revision_and_failure_is_retryable() {
    SettingsCommitModel model;
    AppConfig candidate = AppConfig::defaults();
    model.markDirty(false);
    TEST_ASSERT_TRUE(model.queueOnExit(candidate));
    ConfigCommitRequest first;
    TEST_ASSERT_TRUE(model.take(first));

    model.complete(first.revision, false);
    TEST_ASSERT_TRUE(model.dirty());
    TEST_ASSERT_TRUE(model.queueOnExit(candidate));
    ConfigCommitRequest retry;
    TEST_ASSERT_TRUE(model.take(retry));

    model.complete(retry.revision, true);
    TEST_ASSERT_FALSE(model.dirty());
}

void test_stale_success_does_not_clean_a_newer_edit() {
    SettingsCommitModel model;
    AppConfig candidate = AppConfig::defaults();
    model.markDirty(false);
    TEST_ASSERT_TRUE(model.queueOnExit(candidate));
    ConfigCommitRequest first;
    TEST_ASSERT_TRUE(model.take(first));

    candidate.brightness_percent = 30U;
    model.markDirty(false);
    TEST_ASSERT_TRUE(model.queueOnExit(candidate));
    model.complete(first.revision, true);

    TEST_ASSERT_TRUE(model.dirty());
    ConfigCommitRequest second;
    TEST_ASSERT_TRUE(model.take(second));
    TEST_ASSERT_EQUAL_UINT8(30U, second.candidate.brightness_percent);
    TEST_ASSERT_TRUE(second.revision > first.revision);
}

void test_factory_reset_uses_a_distinct_commit_kind() {
    SettingsCommitModel model;
    model.queueFactoryReset();

    ConfigCommitRequest request;
    TEST_ASSERT_TRUE(model.take(request));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(ConfigCommitKind::FactoryReset),
        static_cast<uint8_t>(request.kind));
    TEST_ASSERT_TRUE(model.dirty());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_unchanged_exit_does_not_queue_a_commit);
    RUN_TEST(test_dirty_exit_queues_the_current_snapshot);
    RUN_TEST(test_repeated_edits_coalesce_to_the_latest_snapshot);
    RUN_TEST(test_success_cleans_matching_revision_and_failure_is_retryable);
    RUN_TEST(test_stale_success_does_not_clean_a_newer_edit);
    RUN_TEST(test_factory_reset_uses_a_distinct_commit_kind);
    return UNITY_END();
}

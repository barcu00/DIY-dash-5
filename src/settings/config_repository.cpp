#include "config_repository.h"

ConfigRepository::ConfigRepository(ConfigBackend& backend) : backend_(backend) {}

LoadResult ConfigRepository::load(AppConfig& config) {
    AppConfig candidate{};
    if (!backend_.read(&candidate, sizeof(candidate)) ||
        candidate.schema_version != AppConfig::kSchemaVersion ||
        !candidate.validate().valid) {
        config = AppConfig::defaults();
        return LoadResult::DefaultsUsed;
    }

    config = candidate;
    return LoadResult::Loaded;
}

bool ConfigRepository::saveCandidate(const AppConfig& candidate,
                                     AppConfig& runtime_config) {
    AppConfig validated = candidate;
    if (validated.schema_version != AppConfig::kSchemaVersion ||
        !validated.validate().valid ||
        !backend_.write(&validated, sizeof(validated))) {
        return false;
    }

    runtime_config = validated;
    return true;
}

bool ConfigRepository::reset(AppConfig& runtime_config) {
    if (!backend_.erase()) {
        return false;
    }
    runtime_config = AppConfig::defaults();
    return true;
}

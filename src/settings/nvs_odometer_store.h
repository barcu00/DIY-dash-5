#pragma once

#include "odometer/odometer_record.h"

// Odometer record on the "dashcfg" partition, in its own namespace so that
// FACTORY RESET (which clears the configuration namespace) keeps it.
class NvsOdometerStore : public OdometerStore {
public:
    OdometerLoadResult load(OdometerRecord& record) override;
    bool save(const OdometerRecord& record) override;

    static constexpr const char* kNamespace = "odometer";
    static constexpr const char* kKey = "record";
};

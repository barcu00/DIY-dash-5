#pragma once

#include <cstddef>
#include <cstdint>

enum class DistanceSource : uint8_t {
    CanSpeed,
    Wheels,
    Gps,
};

enum class TripId : uint8_t {
    A,
    B,
};

// Persisted on its own NVS key, separate from AppConfig, so a FACTORY RESET
// of the dashboard configuration keeps the odometer. Fixed 64-byte layout
// without implicit padding; the CRC covers every byte before `crc`.
struct OdometerRecord {
    static constexpr uint32_t kMagic = 0x314F444FU;  // "ODO1"
    static constexpr uint16_t kVersion = 1U;

    uint32_t magic = kMagic;
    uint16_t version = kVersion;
    uint8_t source = static_cast<uint8_t>(DistanceSource::CanSpeed);
    uint8_t gps_fallback = 1U;
    int16_t correction_permille = 0;
    uint16_t reserved0 = 0U;
    uint32_t reserved1 = 0U;
    uint64_t odometer_dm = 0U;           // decimetres
    uint64_t trip_dm[2] = {0U, 0U};
    uint64_t trip_moving_ms[2] = {0U, 0U};
    uint32_t crc = 0U;
    uint32_t reserved2 = 0U;
};
static_assert(sizeof(OdometerRecord) == 64U, "odometer record layout is persisted");
static_assert(offsetof(OdometerRecord, odometer_dm) == 16U, "odometer record layout is persisted");
static_assert(offsetof(OdometerRecord, crc) == 56U, "odometer record layout is persisted");

uint32_t odometerRecordCrc(const OdometerRecord& record);
void sealOdometerRecord(OdometerRecord& record);
bool odometerRecordValid(const OdometerRecord& record);

enum class OdometerLoadResult : uint8_t {
    Loaded,
    Empty,        // nothing stored yet (first boot)
    Corrupt,      // CRC/version mismatch; overwritten on the next save
    Unavailable,  // storage could not be opened; do not overwrite it
};

class OdometerStore {
public:
    virtual ~OdometerStore() = default;
    virtual OdometerLoadResult load(OdometerRecord& record) = 0;
    virtual bool save(const OdometerRecord& record) = 0;
};

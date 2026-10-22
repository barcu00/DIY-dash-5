#include "odometer_record.h"

namespace {
constexpr int16_t kMaximumCorrectionPermille = 100;

uint32_t crc32(const uint8_t* data, std::size_t size) {
    uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t i = 0U; i < size; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc >> 1U) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}
}  // namespace

uint32_t odometerRecordCrc(const OdometerRecord& record) {
    return crc32(reinterpret_cast<const uint8_t*>(&record),
                 offsetof(OdometerRecord, crc));
}

void sealOdometerRecord(OdometerRecord& record) {
    record.magic = OdometerRecord::kMagic;
    record.version = OdometerRecord::kVersion;
    record.reserved0 = 0U;
    record.reserved1 = 0U;
    record.reserved2 = 0U;
    record.crc = odometerRecordCrc(record);
}

bool odometerRecordValid(const OdometerRecord& record) {
    return record.magic == OdometerRecord::kMagic &&
           record.version == OdometerRecord::kVersion &&
           record.source <= static_cast<uint8_t>(DistanceSource::Gps) &&
           record.gps_fallback <= 1U &&
           record.correction_permille >= -kMaximumCorrectionPermille &&
           record.correction_permille <= kMaximumCorrectionPermille &&
           record.crc == odometerRecordCrc(record);
}

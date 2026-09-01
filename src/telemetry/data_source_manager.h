#pragma once

#include <cstdint>

#include "can/can_frame.h"
#include "data_source.h"
#include "ecu/ecumaster_decoder.h"
#include "mock_telemetry.h"
#include "parameter_registry.h"

class DataSourceManager {
public:
    explicit DataSourceManager(ParameterRegistry& registry);

    void setSource(DataSource source);
    DataSource source() const;

    void setTimeoutMs(uint32_t timeout_ms);
    uint32_t timeoutMs() const;

    void setEcumasterBaseId(uint16_t base_id);
    uint16_t ecumasterBaseId() const;

    bool handleCanFrame(const CanFrame& frame, uint32_t now_ms);
    void update(uint32_t now_ms);

    bool canOnline() const;
    uint32_t receivedFrameCount() const;
    uint32_t invalidFrameCount() const;

private:
    void publishMock(uint32_t now_ms);

    ParameterRegistry& registry_;
    MockTelemetry mock_;
    EcumasterDecoder ecumaster_;
    DataSource source_ = DataSource::Mock;
    uint32_t timeout_ms_ = 500U;
    uint32_t last_valid_can_ms_ = 0U;
    uint32_t received_frames_ = 0U;
    uint32_t invalid_frames_ = 0U;
    bool can_online_ = false;
    bool have_valid_can_frame_ = false;
};

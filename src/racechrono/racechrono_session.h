#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "racechrono_event_queue.h"
#include "racechrono_telemetry.h"

enum class RaceChronoConnectionState : uint8_t {
    Disabled,
    Advertising,
    Connected,
    Configuring,
    Active,
    NoData,
    Error,
};

class RaceChronoSession {
public:
    explicit RaceChronoSession(RaceChronoTelemetry& telemetry);

    void setEnabled(bool enabled);
    void onEvent(const RaceChronoEvent& event, uint32_t now_ms);
    void update(uint32_t now_ms);
    bool takeAction(RaceChronoAction& action);
    RaceChronoConnectionState state() const;

private:
    enum class PendingIndication : uint8_t {
        None,
        RemoveAll,
        AddFragment,
        UpdateAll,
    };

    void clearActions();
    bool enqueue(const RaceChronoAction& action);
    void enqueueSimple(RaceChronoActionType type);
    void enqueueIndication(const RaceChronoPacket& packet,
                           PendingIndication pending);
    void queueCurrentFragment();
    void advanceChannel(uint32_t now_ms);
    void handleIndicationConfirmed(uint32_t now_ms);
    void handleConfigWrite(const RaceChronoEvent& event, uint32_t now_ms);
    void handleValuesWrite(const RaceChronoEvent& event, uint32_t now_ms);

    RaceChronoTelemetry& telemetry_;
    RaceChronoConnectionState state_ = RaceChronoConnectionState::Disabled;
    bool enabled_ = false;
    std::size_t channel_index_ = 0U;
    std::size_t fragment_index_ = 0U;
    uint8_t sequence_retries_ = 0U;
    PendingIndication pending_indication_ = PendingIndication::None;
    uint8_t pending_command_ = 0U;
    bool waiting_config_write_ = false;
    bool have_valid_packet_ = false;
    uint32_t last_valid_packet_ms_ = 0U;
    uint32_t last_update_request_ms_ = 0U;

    std::array<RaceChronoAction, 8U> actions_{};
    uint8_t action_head_ = 0U;
    uint8_t action_tail_ = 0U;
    uint8_t action_count_ = 0U;
};

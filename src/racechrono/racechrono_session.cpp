#include "racechrono_session.h"

RaceChronoSession::RaceChronoSession(RaceChronoTelemetry& telemetry)
    : telemetry_(telemetry) {}

void RaceChronoSession::setEnabled(bool enabled) {
    if (enabled == enabled_) {
        return;
    }
    enabled_ = enabled;
    clearActions();
    pending_indication_ = PendingIndication::None;
    waiting_config_write_ = false;
    have_valid_packet_ = false;
    telemetry_.invalidateAll();

    if (enabled_) {
        state_ = RaceChronoConnectionState::Advertising;
        enqueueSimple(RaceChronoActionType::StartAdvertising);
    } else {
        state_ = RaceChronoConnectionState::Disabled;
        enqueueSimple(RaceChronoActionType::Disconnect);
        enqueueSimple(RaceChronoActionType::StopAdvertising);
    }
}

void RaceChronoSession::onEvent(const RaceChronoEvent& event,
                                uint32_t now_ms) {
    if (!enabled_) {
        return;
    }
    switch (event.type) {
        case RaceChronoEventType::Connected:
            state_ = RaceChronoConnectionState::Connected;
            pending_indication_ = PendingIndication::None;
            waiting_config_write_ = false;
            break;
        case RaceChronoEventType::Disconnected:
            clearActions();
            pending_indication_ = PendingIndication::None;
            waiting_config_write_ = false;
            have_valid_packet_ = false;
            telemetry_.invalidateAll();
            state_ = RaceChronoConnectionState::Advertising;
            enqueueSimple(RaceChronoActionType::StartAdvertising);
            break;
        case RaceChronoEventType::IndicationsSubscribed:
            telemetry_.invalidateAll();
            channel_index_ = 0U;
            fragment_index_ = 0U;
            sequence_retries_ = 0U;
            waiting_config_write_ = false;
            have_valid_packet_ = false;
            state_ = RaceChronoConnectionState::Configuring;
            enqueueIndication(RaceChronoProtocol::removeAll(),
                              PendingIndication::RemoveAll);
            break;
        case RaceChronoEventType::IndicationConfirmed:
            handleIndicationConfirmed(now_ms);
            break;
        case RaceChronoEventType::ConfigWrite:
            handleConfigWrite(event, now_ms);
            break;
        case RaceChronoEventType::ValuesWrite:
            handleValuesWrite(event, now_ms);
            break;
    }
}

void RaceChronoSession::update(uint32_t now_ms) {
    telemetry_.updateStale(now_ms);
    if (state_ != RaceChronoConnectionState::Active &&
        state_ != RaceChronoConnectionState::NoData) {
        return;
    }

    if (have_valid_packet_ && now_ms - last_valid_packet_ms_ > 2500U) {
        state_ = RaceChronoConnectionState::NoData;
    }
    if (pending_indication_ == PendingIndication::None &&
        now_ms - last_update_request_ms_ >= 2000U) {
        last_update_request_ms_ = now_ms;
        enqueueIndication(RaceChronoProtocol::updateAll(),
                          PendingIndication::UpdateAll);
    }
}

bool RaceChronoSession::takeAction(RaceChronoAction& action) {
    if (action_count_ == 0U) {
        return false;
    }
    action = actions_[action_tail_];
    action_tail_ = static_cast<uint8_t>((action_tail_ + 1U) % actions_.size());
    --action_count_;
    return true;
}

RaceChronoConnectionState RaceChronoSession::state() const {
    return state_;
}

void RaceChronoSession::clearActions() {
    action_head_ = 0U;
    action_tail_ = 0U;
    action_count_ = 0U;
}

bool RaceChronoSession::enqueue(const RaceChronoAction& action) {
    if (action_count_ >= actions_.size()) {
        state_ = RaceChronoConnectionState::Error;
        return false;
    }
    actions_[action_head_] = action;
    action_head_ = static_cast<uint8_t>((action_head_ + 1U) % actions_.size());
    ++action_count_;
    return true;
}

void RaceChronoSession::enqueueSimple(RaceChronoActionType type) {
    RaceChronoAction action;
    action.type = type;
    enqueue(action);
}

void RaceChronoSession::enqueueIndication(const RaceChronoPacket& packet,
                                          PendingIndication pending) {
    if (packet.size == 0U) {
        state_ = RaceChronoConnectionState::Error;
        return;
    }
    RaceChronoAction action;
    action.type = RaceChronoActionType::Indicate;
    action.packet = packet;
    if (enqueue(action)) {
        pending_indication_ = pending;
        pending_command_ = packet.bytes[0];
    }
}

void RaceChronoSession::queueCurrentFragment() {
    if (channel_index_ >= raceChronoChannelCount()) {
        return;
    }
    const auto& channel = raceChronoChannelAt(channel_index_);
    enqueueIndication(RaceChronoProtocol::addFragment(
                          channel.monitor_id, channel.equation, fragment_index_),
                      PendingIndication::AddFragment);
}

void RaceChronoSession::advanceChannel(uint32_t now_ms) {
    ++channel_index_;
    fragment_index_ = 0U;
    sequence_retries_ = 0U;
    waiting_config_write_ = false;
    if (channel_index_ < raceChronoChannelCount()) {
        queueCurrentFragment();
    } else {
        last_update_request_ms_ = now_ms;
        enqueueIndication(RaceChronoProtocol::updateAll(),
                          PendingIndication::UpdateAll);
    }
}

void RaceChronoSession::handleIndicationConfirmed(uint32_t) {
    const PendingIndication confirmed = pending_indication_;
    pending_indication_ = PendingIndication::None;
    if (confirmed == PendingIndication::RemoveAll) {
        fragment_index_ = 0U;
        queueCurrentFragment();
    } else if (confirmed == PendingIndication::AddFragment) {
        if (pending_command_ == 2U) {
            ++fragment_index_;
            queueCurrentFragment();
        } else if (pending_command_ == 3U) {
            waiting_config_write_ = true;
        }
    }
}

void RaceChronoSession::handleConfigWrite(const RaceChronoEvent& event,
                                           uint32_t now_ms) {
    if (!waiting_config_write_ || channel_index_ >= raceChronoChannelCount()) {
        return;
    }
    const RaceChronoConfigResult result = RaceChronoProtocol::decodeConfigResult(
        event.bytes.data(), event.size);
    const auto& channel = raceChronoChannelAt(channel_index_);
    if (result.monitor_id != channel.monitor_id ||
        result.type == RaceChronoConfigResultType::Invalid) {
        return;
    }

    waiting_config_write_ = false;
    if (result.type == RaceChronoConfigResultType::Success) {
        telemetry_.markConfigured(channel.parameter);
        advanceChannel(now_ms);
    } else if (result.type ==
               RaceChronoConfigResultType::EquationException) {
        telemetry_.markError(channel.parameter);
        advanceChannel(now_ms);
    } else if (result.type ==
               RaceChronoConfigResultType::PayloadOutOfSequence) {
        ++sequence_retries_;
        if (sequence_retries_ >= 3U) {
            telemetry_.markError(channel.parameter);
            advanceChannel(now_ms);
        } else {
            fragment_index_ = 0U;
            queueCurrentFragment();
        }
    }
}

void RaceChronoSession::handleValuesWrite(const RaceChronoEvent& event,
                                           uint32_t now_ms) {
    const RaceChronoValueBatch batch = RaceChronoProtocol::decodeValues(
        event.bytes.data(), event.size);
    if (telemetry_.acceptBatch(batch, now_ms)) {
        have_valid_packet_ = true;
        last_valid_packet_ms_ = now_ms;
        state_ = RaceChronoConnectionState::Active;
    }
}

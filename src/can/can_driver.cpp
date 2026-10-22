#include "can_driver.h"

#include <cstring>

#include <Arduino.h>
#include <driver/twai.h>
#include <esp_err.h>

#include "config/dashboard_config.h"

namespace {
bool timingForBitrate(uint32_t bitrate, twai_timing_config_t& timing) {
    switch (bitrate) {
        case 125000U:
            timing = TWAI_TIMING_CONFIG_125KBITS();
            return true;
        case 250000U:
            timing = TWAI_TIMING_CONFIG_250KBITS();
            return true;
        case 500000U:
            timing = TWAI_TIMING_CONFIG_500KBITS();
            return true;
        case 1000000U:
            timing = TWAI_TIMING_CONFIG_1MBITS();
            return true;
        default:
            return false;
    }
}
}

CanDriver::~CanDriver() {
    stop();
}

bool CanDriver::begin(uint32_t bitrate) {
    stop();

    twai_timing_config_t timing{};
    if (!timingForBitrate(bitrate, timing)) {
        return false;
    }

    twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(
        static_cast<gpio_num_t>(DashboardConfig::kCanTxGpio),
        static_cast<gpio_num_t>(DashboardConfig::kCanRxGpio),
        TWAI_MODE_LISTEN_ONLY);
    general.tx_queue_len = 0U;
    general.rx_queue_len = DashboardConfig::kCanRxQueueLength;
    const twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&general, &timing, &filter) != ESP_OK) {
        return false;
    }
    if (twai_start() != ESP_OK) {
        twai_driver_uninstall();
        return false;
    }

    bitrate_ = bitrate;
    received_frames_ = 0U;
    rejected_frames_ = 0U;
    health_ = CanHealth{};
    running_ = true;
    return true;
}

bool CanDriver::poll(CanFrame& frame) {
    if (!running_) {
        return false;
    }

    twai_message_t message{};
    if (twai_receive(&message, 0) != ESP_OK) {
        return false;
    }

    if (message.data_length_code > sizeof(frame.data)) {
        ++rejected_frames_;
        return false;
    }

    frame.id = message.identifier;
    frame.dlc = message.data_length_code;
    frame.extended = message.extd;
    frame.remote = message.rtr;
    std::memset(frame.data, 0, sizeof(frame.data));
    if (!message.rtr && message.data_length_code > 0U) {
        std::memcpy(frame.data, message.data, message.data_length_code);
    }
    ++received_frames_;
    return true;
}

void CanDriver::stop() {
    if (running_) {
        twai_stop();
        twai_driver_uninstall();
    }
    running_ = false;
    bitrate_ = 0U;
}

bool CanDriver::running() const {
    return running_;
}

uint32_t CanDriver::bitrate() const {
    return bitrate_;
}

uint32_t CanDriver::receivedFrames() const {
    return received_frames_;
}

uint32_t CanDriver::rejectedFrames() const {
    return rejected_frames_;
}

CanHealth CanDriver::health() const {
    if (!running_) return health_;
    const uint32_t now = millis();
    if (now - health_polled_ms_ < 250U) return health_;
    health_polled_ms_ = now;
    twai_status_info_t info{};
    if (twai_get_status_info(&info) == ESP_OK) {
        health_.dropped_frames = info.rx_missed_count + info.rx_overrun_count;
        health_.bus_errors = info.bus_error_count;
        health_.error_passive = info.rx_error_counter >= 128U;
    }
    return health_;
}

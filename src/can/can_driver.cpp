#include "can_driver.h"

#include <cstring>

#include <driver/twai.h>
#include <esp_err.h>

namespace {
constexpr gpio_num_t kCanTx = GPIO_NUM_15;
constexpr gpio_num_t kCanRx = GPIO_NUM_16;

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

bool CanDriver::begin(uint32_t bitrate) {
    stop();

    twai_timing_config_t timing{};
    if (!timingForBitrate(bitrate, timing)) {
        return false;
    }

    twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(kCanTx, kCanRx, TWAI_MODE_NORMAL);
    general.tx_queue_len = 0;
    general.rx_queue_len = 32;

    const twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    if (twai_driver_install(&general, &timing, &filter) != ESP_OK) {
        return false;
    }

    if (twai_start() != ESP_OK) {
        twai_driver_uninstall();
        return false;
    }

    bitrate_ = bitrate;
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

    frame.id = message.identifier;
    frame.dlc = message.data_length_code > 8U ? 8U : message.data_length_code;
    std::memset(frame.data, 0, sizeof(frame.data));
    if (!message.rtr && frame.dlc > 0U) {
        std::memcpy(frame.data, message.data, frame.dlc);
    }
    frame.extended = message.extd;
    frame.remote = message.rtr;
    return true;
}

void CanDriver::stop() {
    if (!running_) {
        bitrate_ = 0;
        return;
    }

    twai_stop();
    twai_driver_uninstall();
    running_ = false;
    bitrate_ = 0;
}

bool CanDriver::running() const {
    return running_;
}

uint32_t CanDriver::bitrate() const {
    return bitrate_;
}

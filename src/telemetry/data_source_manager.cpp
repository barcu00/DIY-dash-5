#include "data_source_manager.h"

DataSourceManager::DataSourceManager(ParameterRegistry& registry)
    : registry_(registry), ecumaster_(registry) {
    mock_.reset();
}

void DataSourceManager::setSource(DataSource source) {
    if (source_ == source) {
        return;
    }

    source_ = source;
    registry_.invalidateAll();
    can_online_ = false;
    have_valid_can_frame_ = false;
    last_valid_can_ms_ = 0U;

    if (source_ == DataSource::Mock) {
        mock_.reset();
    }
}

DataSource DataSourceManager::source() const {
    return source_;
}

void DataSourceManager::setTimeoutMs(uint32_t timeout_ms) {
    timeout_ms_ = timeout_ms;
}

uint32_t DataSourceManager::timeoutMs() const {
    return timeout_ms_;
}

void DataSourceManager::setEcumasterBaseId(uint16_t base_id) {
    ecumaster_.setBaseId(base_id);
}

uint16_t DataSourceManager::ecumasterBaseId() const {
    return ecumaster_.baseId();
}

bool DataSourceManager::handleCanFrame(const CanFrame& frame, uint32_t now_ms) {
    if (source_ != DataSource::Ecumaster) {
        return false;
    }

    const uint32_t base_id = ecumaster_.baseId();
    if (frame.id < base_id || frame.id > base_id + 7U) {
        return false;
    }

    if (!ecumaster_.decode(frame.id, frame.data, frame.dlc, now_ms)) {
        ++invalid_frames_;
        return false;
    }

    ++received_frames_;
    last_valid_can_ms_ = now_ms;
    have_valid_can_frame_ = true;
    can_online_ = true;
    return true;
}

void DataSourceManager::update(uint32_t now_ms) {
    switch (source_) {
        case DataSource::Mock:
            publishMock(now_ms);
            can_online_ = false;
            break;

        case DataSource::Ecumaster:
            if (have_valid_can_frame_ && static_cast<uint32_t>(now_ms - last_valid_can_ms_) > timeout_ms_) {
                registry_.invalidateAll();
                can_online_ = false;
                have_valid_can_frame_ = false;
            }
            break;

        case DataSource::Rusefi:
            can_online_ = false;
            break;
    }
}

bool DataSourceManager::canOnline() const {
    return can_online_;
}

uint32_t DataSourceManager::receivedFrameCount() const {
    return received_frames_;
}

uint32_t DataSourceManager::invalidFrameCount() const {
    return invalid_frames_;
}

void DataSourceManager::publishMock(uint32_t now_ms) {
    mock_.update(now_ms);
    const VehicleState& s = mock_.state();

    registry_.set(ParameterId::Rpm, static_cast<float>(s.rpm), now_ms);
    registry_.set(ParameterId::Gear, static_cast<float>(s.gear), now_ms);
    registry_.set(ParameterId::VehicleSpeed, s.speed_kph, now_ms);
    registry_.set(ParameterId::Map, s.map_bar, now_ms);
    registry_.set(ParameterId::Lambda, s.lambda, now_ms);
    registry_.set(ParameterId::Clt, s.clt_c, now_ms);
    registry_.set(ParameterId::Iat, s.iat_c, now_ms);
    registry_.set(ParameterId::OilPressure, s.oil_pressure_bar, now_ms);
    registry_.set(ParameterId::OilTemperature, s.oil_temp_c, now_ms);
    registry_.set(ParameterId::FuelPressure, s.fuel_pressure_bar, now_ms);
    registry_.set(ParameterId::BatteryVoltage, s.battery_v, now_ms);
    registry_.set(ParameterId::Tps, s.tps_percent, now_ms);
}

#include "racechrono_ble_transport.h"

#ifdef ARDUINO

#include <NimBLEDevice.h>

#include <algorithm>
#include <cstring>
#include <string>

namespace {
constexpr const char* kDeviceName = "DIY DASH RC";
constexpr const char* kServiceUuid = "1FF8";
constexpr const char* kConfigUuid = "0005";
constexpr const char* kValuesUuid = "0006";
constexpr uint16_t kInvalidConnection = 0xFFFFU;
}

struct RaceChronoBleTransport::Impl {
    explicit Impl(RaceChronoEventQueue<32U>& queue)
        : queue(queue), server_callbacks(*this), config_callbacks(*this),
          values_callbacks(*this) {}

    bool push(RaceChronoEventType type) {
        RaceChronoEvent event;
        event.type = type;
        return queue.pushFromProducer(event);
    }

    bool pushWrite(RaceChronoEventType type,
                   NimBLECharacteristic* characteristic) {
        const std::string value = characteristic->getValue();
        if (value.size() > 20U) return false;
        RaceChronoEvent event;
        event.type = type;
        event.size = static_cast<uint8_t>(value.size());
        if (!value.empty())
            std::memcpy(event.bytes.data(), value.data(), value.size());
        return queue.pushFromProducer(event);
    }

    class ServerCallbacks final : public NimBLEServerCallbacks {
    public:
        explicit ServerCallbacks(Impl& owner) : owner_(owner) {}
        void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
            owner_.connection_handle = info.getConnHandle();
            owner_.push(RaceChronoEventType::Connected);
        }
        void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
            owner_.connection_handle = kInvalidConnection;
            owner_.push(RaceChronoEventType::Disconnected);
        }
    private:
        Impl& owner_;
    };

    class ConfigCallbacks final : public NimBLECharacteristicCallbacks {
    public:
        explicit ConfigCallbacks(Impl& owner) : owner_(owner) {}
        void onWrite(NimBLECharacteristic* characteristic,
                     NimBLEConnInfo&) override {
            owner_.pushWrite(RaceChronoEventType::ConfigWrite,
                             characteristic);
        }
        void onSubscribe(NimBLECharacteristic*, NimBLEConnInfo&,
                         uint16_t subscriptions) override {
            if ((subscriptions & 2U) != 0U)
                owner_.push(RaceChronoEventType::IndicationsSubscribed);
        }
        void onStatus(NimBLECharacteristic*, int code) override {
            if (code == BLE_HS_EDONE)
                owner_.push(RaceChronoEventType::IndicationConfirmed);
        }
    private:
        Impl& owner_;
    };

    class ValuesCallbacks final : public NimBLECharacteristicCallbacks {
    public:
        explicit ValuesCallbacks(Impl& owner) : owner_(owner) {}
        void onWrite(NimBLECharacteristic* characteristic,
                     NimBLEConnInfo&) override {
            owner_.pushWrite(RaceChronoEventType::ValuesWrite,
                             characteristic);
        }
    private:
        Impl& owner_;
    };

    RaceChronoEventQueue<32U>& queue;
    ServerCallbacks server_callbacks;
    ConfigCallbacks config_callbacks;
    ValuesCallbacks values_callbacks;
    NimBLEServer* server = nullptr;
    NimBLECharacteristic* config = nullptr;
    NimBLECharacteristic* values = nullptr;
    uint16_t connection_handle = kInvalidConnection;
    bool initialized = false;
};

RaceChronoBleTransport::RaceChronoBleTransport() = default;

RaceChronoBleTransport::~RaceChronoBleTransport() {
    delete impl_;
}

bool RaceChronoBleTransport::begin() {
    if (impl_ && impl_->initialized) return true;
    if (!impl_) impl_ = new Impl(events_);
    NimBLEDevice::init(kDeviceName);
    impl_->server = NimBLEDevice::createServer();
    if (!impl_->server) return false;
    impl_->server->setCallbacks(&impl_->server_callbacks, false);
    NimBLEService* service = impl_->server->createService(kServiceUuid);
    if (!service) return false;
    impl_->config = service->createCharacteristic(
        kConfigUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::INDICATE,
        20U);
    impl_->values = service->createCharacteristic(
        kValuesUuid, NIMBLE_PROPERTY::WRITE_NR, 20U);
    if (!impl_->config || !impl_->values) return false;
    impl_->config->setCallbacks(&impl_->config_callbacks);
    impl_->values->setCallbacks(&impl_->values_callbacks);
    if (!service->start()) return false;
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(kServiceUuid);
    impl_->initialized = true;
    return true;
}

bool RaceChronoBleTransport::startAdvertising() {
    return impl_ && impl_->initialized &&
           NimBLEDevice::getAdvertising()->start();
}

void RaceChronoBleTransport::stopAdvertising() {
    if (impl_ && impl_->initialized) NimBLEDevice::getAdvertising()->stop();
}

void RaceChronoBleTransport::disconnect() {
    if (!impl_ || !impl_->server ||
        impl_->connection_handle == kInvalidConnection) return;
    impl_->server->disconnect(impl_->connection_handle);
}

bool RaceChronoBleTransport::indicate(const uint8_t* data,
                                      std::size_t size) {
    return impl_ && impl_->config && size <= 20U &&
           impl_->config->indicate(data, size, impl_->connection_handle);
}

bool RaceChronoBleTransport::pollEvent(RaceChronoEvent& event) {
    return events_.popFromConsumer(event);
}

bool RaceChronoBleTransport::restart() {
    disconnect();
    return startAdvertising();
}

#else

RaceChronoBleTransport::RaceChronoBleTransport() = default;
RaceChronoBleTransport::~RaceChronoBleTransport() = default;
bool RaceChronoBleTransport::begin() { return false; }
bool RaceChronoBleTransport::startAdvertising() { return false; }
void RaceChronoBleTransport::stopAdvertising() {}
void RaceChronoBleTransport::disconnect() {}
bool RaceChronoBleTransport::indicate(const uint8_t*, std::size_t) {
    return false;
}
bool RaceChronoBleTransport::pollEvent(RaceChronoEvent& event) {
    return events_.popFromConsumer(event);
}
bool RaceChronoBleTransport::restart() { return false; }

#endif

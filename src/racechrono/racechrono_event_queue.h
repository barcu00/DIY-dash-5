#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "racechrono_protocol.h"

enum class RaceChronoEventType : uint8_t {
    Connected,
    Disconnected,
    IndicationsSubscribed,
    IndicationConfirmed,
    ConfigWrite,
    ValuesWrite,
};

struct RaceChronoEvent {
    RaceChronoEventType type = RaceChronoEventType::Disconnected;
    std::array<uint8_t, 20U> bytes{};
    uint8_t size = 0U;
};

enum class RaceChronoActionType : uint8_t {
    None,
    StartAdvertising,
    StopAdvertising,
    Disconnect,
    Indicate,
};

struct RaceChronoAction {
    RaceChronoActionType type = RaceChronoActionType::None;
    RaceChronoPacket packet{};
};

template <std::size_t Capacity>
class RaceChronoEventQueue {
    static_assert(Capacity >= 2U, "Queue needs one usable slot");

public:
    bool pushFromProducer(const RaceChronoEvent& event) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = increment(head);
        if (next == tail_.load(std::memory_order_acquire)) {
            dropped_.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        events_[head] = event;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool popFromConsumer(RaceChronoEvent& event) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        event = events_[tail];
        tail_.store(increment(tail), std::memory_order_release);
        return true;
    }

    uint32_t droppedCount() const {
        return dropped_.load(std::memory_order_relaxed);
    }

    void clear() {
        tail_.store(head_.load(std::memory_order_acquire),
                    std::memory_order_release);
    }

private:
    static constexpr std::size_t increment(std::size_t index) {
        return (index + 1U) % Capacity;
    }

    std::array<RaceChronoEvent, Capacity> events_{};
    std::atomic<std::size_t> head_{0U};
    std::atomic<std::size_t> tail_{0U};
    std::atomic<uint32_t> dropped_{0U};
};

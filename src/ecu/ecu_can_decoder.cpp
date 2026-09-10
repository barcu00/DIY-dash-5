#include "ecu_can_decoder.h"

#include <array>
#include <cmath>
#include <cstring>

EcuCanDecoder::EcuCanDecoder(const SignalDefinition* definitions,
                             std::size_t count)
    : definitions_(definitions), count_(definitions == nullptr ? 0U : count) {}

EcuCanDecoder::EcuCanDecoder(const CanProfile* profile) : profile_(profile) {}

bool EcuCanDecoder::decode(const CanFrame& frame, VehicleState& state,
                           uint32_t now_ms) const {
    if (frame.remote || frame.dlc > sizeof(frame.data)) {
        return false;
    }

    if (profile_ != nullptr) {
        for (std::size_t frame_index = 0U;
             frame_index < profile_->frame_count; ++frame_index) {
            const CanFrameDefinition& candidate = profile_->frames[frame_index];
            if (candidate.can_id != frame.id ||
                candidate.extended != frame.extended ||
                candidate.expected_dlc != frame.dlc) {
                continue;
            }

            if (candidate.discriminator_mask != 0U) {
                if (candidate.discriminator_offset >= frame.dlc) {
                    continue;
                }
                uint16_t discriminator = frame.data[candidate.discriminator_offset];
                if (candidate.discriminator_mask > 0xFFU) {
                    if (candidate.discriminator_offset + 1U >= frame.dlc) {
                        continue;
                    }
                    discriminator |= static_cast<uint16_t>(
                        frame.data[candidate.discriminator_offset + 1U]) << 8U;
                }
                if ((discriminator & candidate.discriminator_mask) !=
                    candidate.discriminator_value) {
                    continue;
                }
            }

            struct StagedValue {
                VehicleSignal signal;
                float value;
            };
            constexpr std::size_t kMaximumSignalsPerFrame = 24U;
            if (candidate.signal_count > kMaximumSignalsPerFrame) {
                return false;
            }
            std::array<StagedValue, kMaximumSignalsPerFrame> staged{};
            for (std::size_t signal_index = 0U;
                 signal_index < candidate.signal_count; ++signal_index) {
                const CanSignalDefinition& definition =
                    candidate.signals[signal_index];
                const std::size_t width = rawWidth(definition.raw_type);
                if (width == 0U || definition.byte_offset > frame.dlc ||
                    width > static_cast<std::size_t>(
                                frame.dlc - definition.byte_offset)) {
                    return false;
                }
                const uint8_t* raw_data =
                    frame.data + definition.byte_offset;
                float value = 0.0f;
                if (definition.kind == CanSignalKind::MaskedFlag) {
                    const std::size_t width_bits = width * 8U;
                    const uint32_t width_mask = width == sizeof(uint32_t)
                        ? UINT32_MAX
                        : (1U << width_bits) - 1U;
                    if (definition.raw_mask == 0U ||
                        (definition.raw_mask & ~width_mask) != 0U ||
                        definition.raw_shift >= width_bits ||
                        definition.active_values_mask == 0U) {
                        return false;
                    }
                    const uint32_t raw = readUnsigned(
                        raw_data, width, definition.byte_order);
                    const uint32_t extracted =
                        (raw & definition.raw_mask) >> definition.raw_shift;
                    if (extracted >= 64U) {
                        return false;
                    }
                    value = (definition.active_values_mask &
                             (1ULL << extracted)) != 0U
                        ? 1.0f
                        : 0.0f;
                } else if (definition.kind == CanSignalKind::Linear) {
                    const float raw = readRaw(raw_data, definition.raw_type,
                                              definition.byte_order);
                    value = raw * definition.scale + definition.bias;
                } else {
                    return false;
                }
                if (!std::isfinite(value) || value < definition.minimum_native ||
                    value > definition.maximum_native) {
                    return false;
                }
                staged[signal_index] = {definition.signal, value};
            }
            for (std::size_t signal_index = 0U;
                 signal_index < candidate.signal_count; ++signal_index) {
                state.set(staged[signal_index].signal,
                          staged[signal_index].value, now_ms);
            }
            return candidate.signal_count > 0U;
        }
        return false;
    }

    bool decoded = false;
    for (std::size_t i = 0; i < count_; ++i) {
        const SignalDefinition& definition = definitions_[i];
        if (definition.can_id != frame.id || definition.extended != frame.extended) {
            continue;
        }

        const std::size_t width = rawWidth(definition.raw_type);
        if (width == 0U || definition.byte_offset > frame.dlc ||
            width > static_cast<std::size_t>(frame.dlc - definition.byte_offset)) {
            continue;
        }

        const float raw = readRaw(frame.data + definition.byte_offset,
                                  definition.raw_type, definition.byte_order);
        state.set(definition.signal, raw * definition.scale + definition.bias,
                  now_ms);
        decoded = true;
    }
    return decoded;
}

void EcuCanDecoder::selectProfile(const CanProfile* profile) {
    profile_ = profile;
    definitions_ = nullptr;
    count_ = 0U;
}

const CanProfile* EcuCanDecoder::profile() const {
    return profile_;
}

std::size_t EcuCanDecoder::definitionCount() const {
    if (profile_ != nullptr) {
        std::size_t result = 0U;
        for (std::size_t i = 0U; i < profile_->frame_count; ++i) {
            result += profile_->frames[i].signal_count;
        }
        return result;
    }
    return count_;
}

uint32_t EcuCanDecoder::timeoutFor(VehicleSignal signal) const {
    uint32_t timeout_ms = 0U;
    if (profile_ != nullptr) {
        for (std::size_t frame_index = 0U;
             frame_index < profile_->frame_count; ++frame_index) {
            const CanFrameDefinition& frame = profile_->frames[frame_index];
            for (std::size_t signal_index = 0U;
                 signal_index < frame.signal_count; ++signal_index) {
                if (frame.signals[signal_index].signal == signal) {
                    timeout_ms = frame.signals[signal_index].timeout_ms;
                }
            }
        }
        return timeout_ms;
    }
    for (std::size_t i = 0; i < count_; ++i) {
        if (definitions_[i].signal == signal) {
            timeout_ms = definitions_[i].timeout_ms;
        }
    }
    return timeout_ms;
}

std::size_t EcuCanDecoder::rawWidth(RawType type) {
    switch (type) {
        case RawType::Unsigned8:
        case RawType::Signed8:
            return 1U;
        case RawType::Unsigned16:
        case RawType::Signed16:
            return 2U;
        case RawType::Unsigned32:
        case RawType::Signed32:
            return 4U;
    }
    return 0U;
}

uint32_t EcuCanDecoder::readUnsigned(const uint8_t* data, std::size_t width,
                                     ByteOrder order) {
    uint32_t value = 0U;
    for (std::size_t i = 0; i < width; ++i) {
        const std::size_t source = order == ByteOrder::Little ? i : width - 1U - i;
        value |= static_cast<uint32_t>(data[source]) << (8U * i);
    }
    return value;
}

float EcuCanDecoder::readRaw(const uint8_t* data, RawType type,
                             ByteOrder order) {
    const uint32_t value = readUnsigned(data, rawWidth(type), order);
    switch (type) {
        case RawType::Unsigned8:
            return static_cast<float>(static_cast<uint8_t>(value));
        case RawType::Signed8:
            return static_cast<float>(static_cast<int8_t>(value));
        case RawType::Unsigned16:
            return static_cast<float>(static_cast<uint16_t>(value));
        case RawType::Signed16:
            return static_cast<float>(static_cast<int16_t>(value));
        case RawType::Unsigned32:
            return static_cast<float>(value);
        case RawType::Signed32: {
            int32_t signed_value = 0;
            std::memcpy(&signed_value, &value, sizeof(signed_value));
            return static_cast<float>(signed_value);
        }
    }
    return 0.0f;
}

#include "parameter_capabilities.h"

#include <array>

ParameterCapabilities ParameterCapabilities::forSource(
    DataSource source, const CanProfile* profile) {
    ParameterCapabilities result;
    std::array<bool, parameterCount()> available{};

    if (source == DataSource::Demo) {
        available.fill(true);
    } else if (source == DataSource::Can && profile != nullptr) {
        for (std::size_t frame_index = 0U;
             frame_index < profile->frame_count; ++frame_index) {
            const CanFrameDefinition& frame = profile->frames[frame_index];
            for (std::size_t signal_index = 0U;
                 signal_index < frame.signal_count; ++signal_index) {
                const std::size_t index = static_cast<std::size_t>(
                    frame.signals[signal_index].signal);
                if (index < available.size()) {
                    available[index] = true;
                }
            }
        }
    }

    for (std::size_t index = 0U; index < available.size(); ++index) {
        if (available[index]) {
            result.parameters_[result.count_++] =
                static_cast<ParameterId>(index);
        }
    }
    return result;
}

std::size_t ParameterCapabilities::count() const {
    return count_;
}

ParameterId ParameterCapabilities::at(std::size_t index) const {
    return index < count_ ? parameters_[index] : ParameterId::Count;
}

bool ParameterCapabilities::supports(ParameterId parameter) const {
    const std::size_t target = static_cast<std::size_t>(parameter);
    if (target >= parameterCount()) {
        return false;
    }
    for (std::size_t index = 0U; index < count_; ++index) {
        if (parameters_[index] == parameter) {
            return true;
        }
    }
    return false;
}

#include "parameter_options.h"

#include <cstdio>

bool ParameterOptionList::write(char* output, std::size_t capacity) const {
    if (output == nullptr || capacity == 0U) {
        return false;
    }

    output[0] = '\0';
    std::size_t used = 0U;
    for (std::size_t index = 0U; index < count_; ++index) {
        const ParameterDescriptor& descriptor =
            parameterDescriptor(parameters_[index]);
        const char* suffix = index == 0U && first_unavailable_
            ? " (UNAVAILABLE)"
            : "";
        const int written = std::snprintf(
            output + used, capacity - used, "%s%s%s",
            index == 0U ? "" : "\n", descriptor.short_name, suffix);
        if (written < 0 || static_cast<std::size_t>(written) >= capacity - used) {
            output[capacity - 1U] = '\0';
            return false;
        }
        used += static_cast<std::size_t>(written);
    }
    return true;
}

std::size_t ParameterOptionList::count() const {
    return count_;
}

ParameterId ParameterOptionList::parameterAt(std::size_t index) const {
    return index < count_ ? parameters_[index] : ParameterId::Count;
}

ParameterOptionList ParameterOptions::build(DataSource source,
                                            const CanProfile* profile,
                                            ParameterId current) {
    ParameterOptionList result;
    const ParameterCapabilities capabilities =
        ParameterCapabilities::forSource(source, profile);
    const bool current_valid =
        static_cast<std::size_t>(current) < parameterCount();
    if (current_valid && !capabilities.supports(current)) {
        result.parameters_[result.count_++] = current;
        result.first_unavailable_ = true;
    }
    for (std::size_t index = 0U; index < capabilities.count(); ++index) {
        result.parameters_[result.count_++] = capabilities.at(index);
    }
    return result;
}

bool ParameterOptions::write(char* output, std::size_t capacity) {
    return build(DataSource::Demo, nullptr, ParameterId::Rpm)
        .write(output, capacity);
}

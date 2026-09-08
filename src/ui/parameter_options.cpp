#include "parameter_options.h"

#include <cstdio>

bool ParameterOptions::write(char* output, std::size_t capacity) {
    if (output == nullptr || capacity == 0U) {
        return false;
    }

    output[0] = '\0';
    std::size_t used = 0U;
    for (std::size_t index = 0U; index < parameterCount(); ++index) {
        const ParameterDescriptor& descriptor =
            parameterDescriptor(static_cast<ParameterId>(index));
        const int written = std::snprintf(
            output + used, capacity - used, "%s%s",
            index == 0U ? "" : "\n", descriptor.short_name);
        if (written < 0 || static_cast<std::size_t>(written) >= capacity - used) {
            output[capacity - 1U] = '\0';
            return false;
        }
        used += static_cast<std::size_t>(written);
    }
    return true;
}

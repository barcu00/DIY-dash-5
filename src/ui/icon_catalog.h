#pragma once

#include "telemetry/parameter_id.h"
#include "ui/icon_id.h"

class IconCatalog {
public:
    static IconId defaultIcon(ParameterId id);
    static const char* name(IconId id);
};

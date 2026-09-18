#pragma once
#include "settings/app_config.h"
enum class ParameterCategory : uint8_t {Engine,Temperature,Pressure,Flags};
ParameterCategory parameterCategory(ParameterId id);
bool warningFromPresented(ParameterId id,const UnitSettings& units,bool enabled,
    WarningDirection direction,float threshold,float reset,uint16_t delay,
    TileWarningConfig& result);

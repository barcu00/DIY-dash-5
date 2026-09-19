#include "editor_value_model.h"
#include <cmath>
#include "racechrono/racechrono_channel_catalog.h"
ParameterCategory parameterCategory(ParameterId id) {
    if(isRaceChronoParameter(id))return ParameterCategory::RaceChrono;
    const auto& d=parameterDescriptor(id);
    if(d.kind==ParameterKind::Flag)return ParameterCategory::Flags;
    if(d.native_unit==NativeUnit::Celsius)return ParameterCategory::Temperature;
    if(d.native_unit==NativeUnit::Bar)return ParameterCategory::Pressure;
    return ParameterCategory::Engine;
}
bool warningFromPresented(ParameterId id,const UnitSettings& units,bool enabled,
    WarningDirection direction,float threshold,float reset,uint16_t delay,
    TileWarningConfig& result) {
    const float t=UnitPresenter::toNative(id,threshold,units);
    const float r=UnitPresenter::toNative(id,reset,units);
    const float h=direction==WarningDirection::Above ? t-r:r-t;
    if(!std::isfinite(t) || !std::isfinite(r) || t<0 || t>999 ||
       h<-.0001f || h>999 || delay>10000)return false;
    result={enabled,direction,t,h<0 ? 0:h,delay};return true;
}

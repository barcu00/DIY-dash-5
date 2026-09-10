#pragma once

#include "settings/app_config.h"

struct TileEditorDraft {
    TileAddress address{};
    TileConfig tile{};
};

class TileEditorModel {
public:
    bool open(TileAddress address, const AppConfig& config);
    void cancel();
    bool isOpen() const;
    const TileEditorDraft& draft() const;

    void setParameter(ParameterId parameter);
    void setVisible(bool visible);
    void setDecimals(uint8_t decimals);
    void setWarning(const TileWarningConfig& warning);
    void setTemperatureBar(const TemperatureBarConfig& temperature_bar);
    void setFlagActiveColor(FlagActiveColor color);

    bool writeCandidate(AppConfig& config) const;
    bool applyTo(AppConfig& config);

private:
    bool open_ = false;
    TileEditorDraft draft_{};
};

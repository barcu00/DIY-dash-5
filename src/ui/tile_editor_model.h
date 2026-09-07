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

    bool applyTo(AppConfig& config);

private:
    bool open_ = false;
    ParameterId original_parameter_ = ParameterId::Rpm;
    TileEditorDraft draft_{};
};

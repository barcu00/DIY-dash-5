#pragma once

#include <cstdint>
#include <lvgl.h>

#include "settings/app_config.h"
#include "settings/nvs_config_store.h"
#include "telemetry/parameter_registry.h"

class SettingsOverlay {
public:
    using LayoutHandler = void (*)(void* context, bool track);
    using ChangedHandler = void (*)(void* context);

    void begin(AppConfig& config, ParameterRegistry& registry, NvsConfigStore& store,
               void* context, LayoutHandler layout_handler, ChangedHandler changed_handler);
    void bindSectionCard(lv_obj_t* card, uint8_t section_index);
    void openWarning(ParameterId parameter);
    bool active() const { return overlay_ != nullptr; }
    void close();

private:
    struct CardContext {
        SettingsOverlay* owner = nullptr;
        uint8_t section = 0U;
    };

    static void cardEvent(lv_event_t* event);
    static void closeEvent(lv_event_t* event);
    static void canSourceEvent(lv_event_t* event);
    static void canBitrateEvent(lv_event_t* event);
    static void canBaseDownEvent(lv_event_t* event);
    static void canBaseUpEvent(lv_event_t* event);
    static void canTimeoutDownEvent(lv_event_t* event);
    static void canTimeoutUpEvent(lv_event_t* event);
    static void parameterPrevEvent(lv_event_t* event);
    static void parameterNextEvent(lv_event_t* event);
    static void parameterToggleEvent(lv_event_t* event);
    static void warningPrevEvent(lv_event_t* event);
    static void warningNextEvent(lv_event_t* event);
    static void warningModeEvent(lv_event_t* event);
    static void warningWarnDownEvent(lv_event_t* event);
    static void warningWarnUpEvent(lv_event_t* event);
    static void warningCriticalDownEvent(lv_event_t* event);
    static void warningCriticalUpEvent(lv_event_t* event);
    static void warningHystDownEvent(lv_event_t* event);
    static void warningHystUpEvent(lv_event_t* event);
    static void warningDelayDownEvent(lv_event_t* event);
    static void warningDelayUpEvent(lv_event_t* event);
    static void warningCurvePointEvent(lv_event_t* event);
    static void warningCurveAddEvent(lv_event_t* event);
    static void warningCurveRemoveEvent(lv_event_t* event);
    static void warningCurveRpmDownEvent(lv_event_t* event);
    static void warningCurveRpmUpEvent(lv_event_t* event);
    static void stoichDownEvent(lv_event_t* event);
    static void stoichUpEvent(lv_event_t* event);
    static void lambdaFormatEvent(lv_event_t* event);
    static void factoryResetEvent(lv_event_t* event);

    void openSection(uint8_t section);
    void openCan();
    void openParameters();
    void openWarnings();
    void openFuel();
    void openSystem();
    void openInfo(const char* title, const char* message);
    lv_obj_t* createOverlay(const char* title);
    lv_obj_t* button(const char* text, int x, int y, int w, lv_event_cb_t callback);
    lv_obj_t* text(const char* value, int x, int y, const lv_font_t* font,
                   lv_color_t color = lv_color_white());
    void save();
    void notifyChanged();
    void refreshRegistryVisibility();
    void applyGlobalLambdaFormat();
    WarningConfig* selectedWarning();
    float warningStep() const;
    static const char* warningModeName(WarningMode mode);
    void adjustWarningScalar(float delta, bool critical);

    AppConfig* config_ = nullptr;
    ParameterRegistry* registry_ = nullptr;
    NvsConfigStore* store_ = nullptr;
    void* context_ = nullptr;
    LayoutHandler layout_handler_ = nullptr;
    ChangedHandler changed_handler_ = nullptr;
    lv_obj_t* overlay_ = nullptr;
    uint16_t selected_parameter_ = 0U;
    uint16_t selected_warning_parameter_ = 0U;
    uint8_t selected_curve_point_ = 0U;
    CardContext card_contexts_[7]{};
};

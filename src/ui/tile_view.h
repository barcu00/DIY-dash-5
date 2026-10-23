#pragma once
#include <array>
#include <cstdint>
#include <lvgl.h>
#include "settings/app_config.h"
#include "telemetry/composite_telemetry_view.h"
#include "ui/display_signal_filter.h"
#include "ui/flag_tile_model.h"
#include "ui/gauge_model.h"
#include "ui/tile_value_format.h"
#include "ui/trend_model.h"
#include "ui/tile_layout.h"
#include "ui/temperature_bar_model.h"
class TileView {
public:
    void create(lv_obj_t* parent, TileAddress address, lv_event_cb_t callback);
    void apply(const TileConfig& config, const TileGeometry& geometry);
    void hide();
    void update(const TileConfig& config, const UnitSettings& units,
                const CompositeTelemetryView& state, bool supported,
                bool warning_active,
                uint32_t now_ms, bool data_lost = false);
    TileAddress address() const;
    // Frees the ENDURANCE trend buffer while this tile's page is not shown;
    // it is allocated again on the next sample.
    void releaseTrend() { trend_.release(); }
    // ENDURANCE trend window (seconds), applied on the next apply().
    static void setTrendWindowSeconds(uint16_t seconds) { trend_window_s_ = seconds; }
private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* stripe_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* value_ = nullptr;
    lv_obj_t* unit_ = nullptr;
    lv_obj_t* temperature_bar_ = nullptr;
    lv_obj_t* gauge_ = nullptr;
    GaugeRange gauge_range_{};
    uint16_t gauge_fill_ = 0U;
    bool gauge_valid_ = false;
    bool gauge_celsius_ = false;
    float gauge_label_min_ = 0.0f;
    float gauge_label_max_ = 100.0f;
    std::array<char, 8> gauge_unit_{};
    DeltaTone last_tone_ = DeltaTone::Neutral;
    static void drawGauge(lv_event_t* event);
    // Analog-style (chamfered / dial) presentation.
    static void drawChamfer(lv_event_t* event);
    // ENDURANCE rows: value history sparkline with a direction arrow.
    static void drawTrend(lv_event_t* event);
    void arrangeTrendRow();
    lv_obj_t* trend_view_ = nullptr;
    static inline uint16_t trend_window_s_ = 15U;
    TrendHistory trend_{};
    bool trend_mode_ = false;
    float trend_marker_ = 0.0f;
    bool trend_has_marker_ = false;
    float trend_span_ = 1.0f;
    void setupStyled(const TileConfig& config);
    void arrangeStyled();
    uint8_t chamfer_ = 0U;
    bool styled_ = false;
    std::array<char, 32> styled_shape_{};
    bool styled_warning_ = false;
    bool styled_data_lost_ = false;
    const lv_font_t* styled_value_font_ = nullptr;
    TileAddress address_{};
    bool base_border_ = true;
    TileSize size_ = TileSize::Small;
    TileGeometry geometry_{};
    void arrangeUnit(const char* text, bool is_flag);
    void arrangeRow(bool is_flag);
    const lv_font_t* default_value_font_ = nullptr;
    DisplaySignalFilter display_filter_{};
    std::array<char, 32> last_value_text_{};
    std::array<char, 16> last_unit_text_{};
    uint32_t next_refresh_ms_ = 0U;
    bool refresh_initialized_ = false;
    bool value_text_initialized_ = false;
    bool unit_text_initialized_ = false;
    int last_row_border_ = -1;
    bool raw_valid_initialized_ = false;
    bool last_raw_valid_ = false;
    bool warning_initialized_ = false;
    bool last_warning_active_ = false;
    bool last_data_lost_ = false;
    bool flag_style_initialized_ = false;
    FlagTileState last_flag_state_ = FlagTileState::Unavailable;
    uint32_t last_flag_background_ = 0U;
    uint32_t last_flag_pill_ = 0U;
    bool temperature_bar_initialized_ = false;
    uint16_t last_temperature_fill_ = 0U;
    TemperatureBarZone last_temperature_zone_ =
        TemperatureBarZone::Unavailable;
};

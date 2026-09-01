#include "ui.h"

#include <cstdio>

#include "ui/icon_assets.h"
#include "ui/icon_catalog.h"
#include "ui/settings_screen_model.h"
#include "ui/tile_engine.h"

Ui* Ui::instance_ = nullptr;

namespace {
const lv_color_t kBg = lv_color_hex(0x07090D);
const lv_color_t kPanel = lv_color_hex(0x11161D);
const lv_color_t kText = lv_color_hex(0xF2F5F7);
const lv_color_t kMuted = lv_color_hex(0x7D8996);
const lv_color_t kBlue = lv_color_hex(0x20A4F3);
const lv_color_t kGreen = lv_color_hex(0x39D353);
const lv_color_t kYellow = lv_color_hex(0xFFCF33);
const lv_color_t kRed = lv_color_hex(0xFF3B30);
const lv_color_t kPurple = lv_color_hex(0xB26BFF);

const char* dataSourceName(DataSource source) {
    switch (source) {
        case DataSource::Mock: return "MOCK";
        case DataSource::Ecumaster: return "ECUMASTER";
        case DataSource::Rusefi: return "RUSEFI";
    }
    return "UNKNOWN";
}

void styleScreen(lv_obj_t* screen) {
    lv_obj_set_style_bg_color(screen, kBg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, kText, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

lv_obj_t* label(lv_obj_t* parent, const char* text, int x, int y,
                const lv_font_t* font, lv_color_t color = kText) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color, 0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return obj;
}

GestureDirection gestureFromLvgl(lv_dir_t dir) {
    switch (dir) {
        case LV_DIR_LEFT: return GestureDirection::Left;
        case LV_DIR_RIGHT: return GestureDirection::Right;
        case LV_DIR_TOP: return GestureDirection::Up;
        case LV_DIR_BOTTOM: return GestureDirection::Down;
        default: return GestureDirection::None;
    }
}

lv_color_t tileAccent(ParameterId id) {
    switch (id) {
        case ParameterId::Lambda:
        case ParameterId::LambdaTarget:
        case ParameterId::LambdaCorrection:
            return kPurple;
        case ParameterId::OilPressure:
        case ParameterId::OilTemperature:
            return kYellow;
        case ParameterId::FuelPressure:
        case ParameterId::EthanolContent:
        case ParameterId::Tps:
        case ParameterId::DbwPosition:
        case ParameterId::DbwTarget:
            return kGreen;
        case ParameterId::BatteryVoltage:
        case ParameterId::ErrorFlagRaw:
            return kRed;
        default:
            return kBlue;
    }
}
}

void Ui::begin(AppConfig& config, ParameterRegistry& registry, NvsConfigStore& config_store) {
    instance_ = this;
    config_ = &config;
    registry_ = &registry;
    config_store_ = &config_store;

    createDash();
    createTrack();
    createDiag();
    createSettings();
    refreshAllTiles();
    load(UiPage::Dash);
}

void Ui::createShiftBar(lv_obj_t* parent, lv_obj_t** segments) {
    for (int i = 0; i < 12; ++i) {
        segments[i] = lv_obj_create(parent);
        lv_obj_set_pos(segments[i], 14 + i * 64, 10);
        lv_obj_set_size(segments[i], 52, 16);
        lv_obj_set_style_border_width(segments[i], 0, 0);
        lv_obj_set_style_radius(segments[i], 4, 0);
        lv_obj_set_style_bg_color(segments[i], lv_color_hex(0x202830), 0);
        lv_obj_clear_flag(segments[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(segments[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
    }
}

void Ui::createConfigurableTiles(lv_obj_t* parent,
                                 std::array<TileView, AppConfig::kTileCount>& views,
                                 bool track) {
    for (uint8_t i = 0; i < AppConfig::kTileCount; ++i) {
        TileView& view = views[i];
        view.config_index = i;
        view.track = track;

        view.container = lv_obj_create(parent);
        lv_obj_set_size(view.container, 200, 60);
        lv_obj_set_style_bg_color(view.container, kPanel, 0);
        lv_obj_set_style_border_width(view.container, 1, 0);
        lv_obj_set_style_border_color(view.container, lv_color_hex(0x27313C), 0);
        lv_obj_set_style_radius(view.container, 8, 0);
        lv_obj_set_style_pad_all(view.container, 6, 0);
        lv_obj_clear_flag(view.container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(view.container, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(view.container, tileEvent, LV_EVENT_LONG_PRESSED, &view);

        view.title = label(view.container, "--", 8, 1, &lv_font_montserrat_12, kMuted);
        view.value = label(view.container, "--", 8, 22, &lv_font_montserrat_20, kText);

        view.icon = lv_canvas_create(view.container);
        lv_canvas_set_buffer(view.icon, view.icon_buffer.data(), 16, 16, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_pos(view.icon, 170, 7);
        lv_obj_add_flag(view.icon, LV_OBJ_FLAG_GESTURE_BUBBLE);
    }
}

void Ui::createDash() {
    dash_ = lv_obj_create(nullptr);
    styleScreen(dash_);
    lv_obj_add_event_cb(dash_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    createShiftBar(dash_, dash_shift_);

    label(dash_, "RPM", 330, 48, &lv_font_montserrat_14, kMuted);
    dash_rpm_ = label(dash_, "0000", 286, 68, &lv_font_montserrat_48);
    label(dash_, "GEAR", 412, 48, &lv_font_montserrat_14, kMuted);
    dash_gear_ = label(dash_, "N", 432, 66, &lv_font_montserrat_48, kYellow);
    dash_source_ = label(dash_, "DATA SOURCE --", 250, 150, &lv_font_montserrat_14, kMuted);
    label(dash_, "Long-press a tile to edit", 268, 405, &lv_font_montserrat_12, kMuted);

    createConfigurableTiles(dash_, dash_tiles_, false);
}

void Ui::createTrack() {
    track_ = lv_obj_create(nullptr);
    styleScreen(track_);
    lv_obj_add_event_cb(track_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    createShiftBar(track_, track_shift_);

    label(track_, "RPM", 330, 44, &lv_font_montserrat_14, kMuted);
    track_rpm_ = label(track_, "0000", 286, 62, &lv_font_montserrat_48);
    label(track_, "GEAR", 412, 44, &lv_font_montserrat_14, kMuted);
    track_gear_ = label(track_, "N", 432, 60, &lv_font_montserrat_48, kYellow);

    label(track_, "LAP TIME", 326, 158, &lv_font_montserrat_14, kMuted);
    label(track_, "00:48.73", 278, 184, &lv_font_montserrat_36);
    label(track_, "BEST  01:35.42", 304, 244, &lv_font_montserrat_14, kPurple);
    label(track_, "DELTA -0.38", 323, 270, &lv_font_montserrat_14, kGreen);
    label(track_, "Timing remains MOCK in v0.2", 278, 405, &lv_font_montserrat_12, kMuted);

    createConfigurableTiles(track_, track_tiles_, true);
}

void Ui::createDiag() {
    diag_ = lv_obj_create(nullptr);
    styleScreen(diag_);
    lv_obj_add_event_cb(diag_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    label(diag_, "OPENDASH DIAGNOSTICS", 22, 18, &lv_font_montserrat_28);
    diag_source_ = label(diag_, "v0.2 / DATA SOURCE: --", 24, 56, &lv_font_montserrat_14, kMuted);

    diag_status_ = label(diag_, "Display: --\nTouch: --\nResolution: 800x480", 30, 110, &lv_font_montserrat_20);
    diag_memory_ = label(diag_, "PSRAM: --\nFree heap: --", 315, 110, &lv_font_montserrat_20);
    diag_runtime_ = label(diag_, "Uptime: --\nUI updates: --", 570, 110, &lv_font_montserrat_20);

    diag_can_box_ = lv_obj_create(diag_);
    lv_obj_set_pos(diag_can_box_, 30, 265);
    lv_obj_set_size(diag_can_box_, 740, 105);
    lv_obj_set_style_bg_color(diag_can_box_, lv_color_hex(0x25120F), 0);
    lv_obj_set_style_border_color(diag_can_box_, kRed, 0);
    lv_obj_set_style_border_width(diag_can_box_, 2, 0);
    lv_obj_set_style_radius(diag_can_box_, 8, 0);
    lv_obj_clear_flag(diag_can_box_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(diag_can_box_, LV_OBJ_FLAG_GESTURE_BUBBLE);
    label(diag_can_box_, "CAN / ECU DATA", 20, 12, &lv_font_montserrat_16, kMuted);
    diag_can_state_ = label(diag_can_box_, "OFFLINE", 20, 38, &lv_font_montserrat_28, kRed);
    diag_can_stats_ = label(diag_can_box_, "1000 kbit/s  RX: 0  BAD: 0", 300, 45, &lv_font_montserrat_16, kMuted);
}

void Ui::createSettings() {
    settings_ = lv_obj_create(nullptr);
    styleScreen(settings_);
    lv_obj_add_event_cb(settings_, gestureEvent, LV_EVENT_GESTURE, nullptr);

    label(settings_, "OPENDASH SETTINGS", 24, 18, &lv_font_montserrat_28);
    label(settings_, "Touch a section to configure", 26, 56, &lv_font_montserrat_14, kMuted);

    for (uint8_t i = 0; i < SettingsScreenModel::sectionCount(); ++i) {
        const int col = i % 2;
        const int row = i / 2;
        lv_obj_t* card = lv_obj_create(settings_);
        lv_obj_set_pos(card, 24 + col * 382, 92 + row * 86);
        lv_obj_set_size(card, 360, 70);
        lv_obj_set_style_bg_color(card, kPanel, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x27313C), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE);
        label(card, SettingsScreenModel::sectionLabel(i), 16, 18, &lv_font_montserrat_20,
              i == 0 ? kBlue : kText);
    }
}

void Ui::refreshTileLayout(std::array<TileView, AppConfig::kTileCount>& views,
                           const std::array<TileConfig, AppConfig::kTileCount>& config) {
    std::array<uint8_t, AppConfig::kTileCount> order{};
    const uint8_t visible_count = TileEngine::visibleOrder(config, order);

    for (auto& view : views) {
        lv_obj_add_flag(view.container, LV_OBJ_FLAG_HIDDEN);
    }

    for (uint8_t slot = 0; slot < visible_count; ++slot) {
        const uint8_t tile_index = order[slot];
        TileView& view = views[tile_index];
        const uint8_t side_slot = static_cast<uint8_t>(slot % 6U);
        const int x = slot < 6U ? 10 : 590;
        const int y = 42 + static_cast<int>(side_slot) * 66;
        lv_obj_set_pos(view.container, x, y);
        lv_obj_clear_flag(view.container, LV_OBJ_FLAG_HIDDEN);
    }
}

void Ui::renderTileIcon(TileView& view, const TileConfig& config) {
    if (registry_ == nullptr || !config.icon_enabled) {
        lv_obj_add_flag(view.icon, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    IconId icon = config.icon == 0U
                      ? registry_->descriptor(config.parameter).default_icon
                      : static_cast<IconId>(config.icon);
    const IconAsset& asset = IconAssets::get(icon);
    if (asset.rows == nullptr || asset.width != 16U || asset.height != 16U) {
        lv_obj_add_flag(view.icon, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    const lv_color_t accent = tileAccent(config.parameter);
    lv_canvas_fill_bg(view.icon, kPanel, LV_OPA_COVER);
    for (uint8_t y = 0; y < 16U; ++y) {
        const uint16_t row = asset.rows[y];
        for (uint8_t x = 0; x < 16U; ++x) {
            if ((row & static_cast<uint16_t>(1U << (15U - x))) != 0U) {
                lv_canvas_set_px_color(view.icon, x, y, accent);
            }
        }
    }
    lv_obj_clear_flag(view.icon, LV_OBJ_FLAG_HIDDEN);
}

void Ui::refreshTileValues(std::array<TileView, AppConfig::kTileCount>& views,
                           const std::array<TileConfig, AppConfig::kTileCount>& config) {
    if (registry_ == nullptr || config_ == nullptr) return;

    for (uint8_t i = 0; i < AppConfig::kTileCount; ++i) {
        TileView& view = views[i];
        const TileConfig& tile = config[i];
        if (!tile.visible) continue;

        const ParameterDescriptor& descriptor = registry_->descriptor(tile.parameter);
        const char* title_text = tile.custom_label_enabled && tile.custom_label[0] != '\0'
                                     ? tile.custom_label.data()
                                     : descriptor.short_name;
        lv_label_set_text(view.title, title_text);
        lv_obj_set_style_text_color(view.title, tileAccent(tile.parameter), 0);

        const ParameterValue& value = registry_->value(tile.parameter);
        if (!value.valid) {
            lv_label_set_text(view.value, "--");
        } else {
            const float presented = TileEngine::presentValue(tile, value.value, config_->stoich_afr);
            char value_text[32];
            std::snprintf(value_text, sizeof(value_text), "%.*f",
                          static_cast<int>(tile.decimals), static_cast<double>(presented));
            lv_label_set_text(view.value, value_text);
        }

        renderTileIcon(view, tile);
    }
}

void Ui::refreshAllTiles() {
    if (config_ == nullptr) return;
    refreshTileLayout(dash_tiles_, config_->tiles);
    refreshTileLayout(track_tiles_, config_->track_tiles);
    refreshTileValues(dash_tiles_, config_->tiles);
    refreshTileValues(track_tiles_, config_->track_tiles);
}

void Ui::updateShiftBar(lv_obj_t** segments, uint16_t rpm) {
    const int lit = static_cast<int>((static_cast<uint32_t>(rpm) * 12U) / 8000U);
    for (int i = 0; i < 12; ++i) {
        lv_color_t color = lv_color_hex(0x202830);
        if (i < lit) color = i < 6 ? kGreen : (i < 9 ? kYellow : kRed);
        lv_obj_set_style_bg_color(segments[i], color, 0);
    }
}

void Ui::update(const VehicleState& s, const RuntimeDiagnostics& d,
                const TelemetryRuntimeStatus& telemetry) {
    char buf[128];

    lv_label_set_text_fmt(dash_rpm_, "%u", s.rpm);
    lv_label_set_text_fmt(dash_gear_, "%d", static_cast<int>(s.gear));
    updateShiftBar(dash_shift_, s.rpm);

    if (telemetry.source == DataSource::Mock) {
        lv_label_set_text(dash_source_, "MOCK TELEMETRY");
        lv_obj_set_style_text_color(dash_source_, kMuted, 0);
    } else {
        std::snprintf(buf, sizeof(buf), "%s CAN / %s", dataSourceName(telemetry.source),
                      telemetry.can_online ? "ONLINE" : "OFFLINE");
        lv_label_set_text(dash_source_, buf);
        lv_obj_set_style_text_color(dash_source_, telemetry.can_online ? kGreen : kRed, 0);
    }

    refreshTileValues(dash_tiles_, config_->tiles);
    refreshTileValues(track_tiles_, config_->track_tiles);

    std::snprintf(buf, sizeof(buf), "Display: %s\nTouch: %s\nResolution: 800x480",
                  d.display_ok ? "OK" : "FAIL", d.touch_ok ? "OK" : "NO");
    lv_label_set_text(diag_status_, buf);
    std::snprintf(buf, sizeof(buf), "PSRAM: %.1f MB\nFree heap: %u KB",
                  static_cast<double>(d.psram_total) / (1024.0 * 1024.0),
                  static_cast<unsigned>(d.free_heap / 1024U));
    lv_label_set_text(diag_memory_, buf);
    std::snprintf(buf, sizeof(buf), "Uptime: %u s\nUI updates: %u",
                  static_cast<unsigned>(d.uptime_ms / 1000U),
                  static_cast<unsigned>(d.ui_updates));
    lv_label_set_text(diag_runtime_, buf);
    std::snprintf(buf, sizeof(buf), "v0.2 / DATA SOURCE: %s", dataSourceName(telemetry.source));
    lv_label_set_text(diag_source_, buf);

    const bool online = telemetry.can_driver_running && telemetry.can_online;
    const bool driver_ready = telemetry.can_driver_running;
    lv_label_set_text(diag_can_state_, online ? "ONLINE" : (driver_ready ? "WAITING FOR ECU" : "DRIVER ERROR"));
    const lv_color_t can_color = online ? kGreen : (driver_ready ? kYellow : kRed);
    lv_obj_set_style_text_color(diag_can_state_, can_color, 0);
    lv_obj_set_style_border_color(diag_can_box_, can_color, 0);
    lv_obj_set_style_bg_color(diag_can_box_, online ? lv_color_hex(0x0D2416) : lv_color_hex(0x25120F), 0);

    std::snprintf(buf, sizeof(buf), "%u kbit/s  RX: %u  BAD: %u",
                  static_cast<unsigned>(telemetry.can_bitrate / 1000U),
                  static_cast<unsigned>(telemetry.received_frames),
                  static_cast<unsigned>(telemetry.invalid_frames));
    lv_label_set_text(diag_can_stats_, buf);

    lv_label_set_text_fmt(track_rpm_, "%u", s.rpm);
    lv_label_set_text_fmt(track_gear_, "%d", static_cast<int>(s.gear));
    updateShiftBar(track_shift_, s.rpm);
}

void Ui::tileEvent(lv_event_t* event) {
    if (instance_ == nullptr || lv_event_get_code(event) != LV_EVENT_LONG_PRESSED) return;
    auto* view = static_cast<TileView*>(lv_event_get_user_data(event));
    if (view != nullptr) instance_->openTileEditor(*view);
}

void Ui::openTileEditor(TileView& view) {
    closeTileEditor();
    editing_tile_ = &view;

    tile_editor_overlay_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(tile_editor_overlay_, 560, 300);
    lv_obj_center(tile_editor_overlay_);
    lv_obj_set_style_bg_color(tile_editor_overlay_, lv_color_hex(0x0C1117), 0);
    lv_obj_set_style_bg_opa(tile_editor_overlay_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tile_editor_overlay_, kBlue, 0);
    lv_obj_set_style_border_width(tile_editor_overlay_, 2, 0);
    lv_obj_set_style_radius(tile_editor_overlay_, 12, 0);
    lv_obj_clear_flag(tile_editor_overlay_, LV_OBJ_FLAG_SCROLLABLE);

    const auto& profile = view.track ? config_->track_tiles : config_->tiles;
    const TileConfig& tile = profile[view.config_index];
    const ParameterDescriptor& descriptor = registry_->descriptor(tile.parameter);

    char title_text[64];
    std::snprintf(title_text, sizeof(title_text), "%s TILE %u",
                  view.track ? "TRACK" : "DASH",
                  static_cast<unsigned>(view.config_index + 1U));
    label(tile_editor_overlay_, title_text, 22, 16, &lv_font_montserrat_24);
    label(tile_editor_overlay_, descriptor.name, 24, 58, &lv_font_montserrat_18, kBlue);
    label(tile_editor_overlay_, "Editor controls are being enabled in Task 9", 24, 104,
          &lv_font_montserrat_14, kMuted);
    label(tile_editor_overlay_, "Parameter / visibility / label / icon / Lambda-AFR / warning", 24, 136,
          &lv_font_montserrat_14, kText);

    lv_obj_t* close = lv_btn_create(tile_editor_overlay_);
    lv_obj_set_pos(close, 390, 220);
    lv_obj_set_size(close, 130, 48);
    lv_obj_set_style_bg_color(close, lv_color_hex(0x153B57), 0);
    lv_obj_add_event_cb(close, [](lv_event_t*) {
        if (Ui::instance_ != nullptr) Ui::instance_->closeTileEditor();
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* close_label = lv_label_create(close);
    lv_label_set_text(close_label, "CLOSE");
    lv_obj_center(close_label);
}

void Ui::closeTileEditor() {
    if (tile_editor_overlay_ != nullptr) {
        lv_obj_del(tile_editor_overlay_);
        tile_editor_overlay_ = nullptr;
    }
    editing_tile_ = nullptr;
}

void Ui::saveConfig() {
    if (config_ != nullptr && config_store_ != nullptr) {
        config_->validate();
        config_store_->save(*config_);
        refreshAllTiles();
    }
}

void Ui::gestureEvent(lv_event_t* event) {
    if (instance_ == nullptr || lv_event_get_code(event) != LV_EVENT_GESTURE ||
        instance_->tile_editor_overlay_ != nullptr) return;
    lv_indev_t* indev = lv_indev_get_act();
    if (indev == nullptr) return;
    const GestureDirection direction = gestureFromLvgl(lv_indev_get_gesture_dir(indev));
    const UiPage next = GestureNavigation::navigate(instance_->current_page_, direction);
    if (next != instance_->current_page_) instance_->load(next);
}

void Ui::load(UiPage page) {
    closeTileEditor();
    current_page_ = page;
    switch (page) {
        case UiPage::Dash: lv_scr_load(dash_); break;
        case UiPage::Track: lv_scr_load(track_); break;
        case UiPage::Diag: lv_scr_load(diag_); break;
        case UiPage::Settings: lv_scr_load(settings_); break;
    }
}

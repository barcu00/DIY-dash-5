#include "ui.h"

#include <cstdio>

#include "ui/icon_assets.h"
#include "ui/icon_catalog.h"
#include "ui/parameter_picker_model.h"
#include "ui/settings_screen_model.h"
#include "ui/tile_editor_model.h"
#include "ui/tile_engine.h"
#include "ui/tile_layout.h"
#include "ui/tile_slot_navigation.h"

Ui* Ui::instance_ = nullptr;

namespace {
const lv_color_t kBg = lv_color_hex(0x07090D);
const lv_color_t kPanel = lv_color_hex(0x0F151C);
const lv_color_t kPanelAlt = lv_color_hex(0x121A22);
const lv_color_t kLine = lv_color_hex(0x2B3540);
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

lv_obj_t* panel(lv_obj_t* parent, int x, int y, int w, int h,
                lv_color_t bg = kPanel, lv_color_t border = kLine) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, bg, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, border, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 9, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return obj;
}

lv_obj_t* actionButton(lv_obj_t* parent, const char* text, int x, int y, int w,
                       lv_color_t color, lv_event_cb_t cb = nullptr) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, 42);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 7, 0);
    if (cb != nullptr) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* txt = lv_label_create(btn);
    lv_label_set_text(txt, text);
    lv_obj_set_style_text_font(txt, &lv_font_montserrat_14, 0);
    lv_obj_center(txt);
    return btn;
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
        case ParameterId::IgnitionAngle:
        case ParameterId::Dwell:
            return kYellow;
        case ParameterId::FuelPressure:
        case ParameterId::EthanolContent:
        case ParameterId::FuelUsed:
        case ParameterId::FuelPumpActive:
        case ParameterId::Tps:
        case ParameterId::DbwPosition:
        case ParameterId::DbwTarget:
            return kGreen;
        case ParameterId::EcuError:
        case ParameterId::CltSensorError:
        case ParameterId::IatSensorError:
        case ParameterId::MapSensorError:
        case ParameterId::WidebandError:
        case ParameterId::Egt1SensorError:
        case ParameterId::Egt2SensorError:
        case ParameterId::EgtAlarm:
        case ParameterId::Knocking:
        case ParameterId::FlexFuelSensorError:
        case ParameterId::DbwError:
        case ParameterId::FuelPressureRelativeError:
        case ParameterId::BatteryVoltage:
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
    settings_overlay_.begin(config, registry, config_store, this,
                            layoutSettingsHandler, settingsChangedHandler);

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
    const TilePlacement geometry = TileLayout::placement(0U);
    for (uint8_t i = 0; i < AppConfig::kTileCount; ++i) {
        TileView& view = views[i];
        view.config_index = i;
        view.track = track;

        view.container = lv_obj_create(parent);
        lv_obj_set_size(view.container, geometry.width, geometry.height);
        lv_obj_set_style_bg_color(view.container, kPanel, 0);
        lv_obj_set_style_bg_opa(view.container, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(view.container, 1, 0);
        lv_obj_set_style_border_color(view.container, kLine, 0);
        lv_obj_set_style_radius(view.container, 8, 0);
        lv_obj_set_style_pad_all(view.container, 0, 0);
        lv_obj_clear_flag(view.container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(view.container, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(view.container, tileEvent, LV_EVENT_LONG_PRESSED, &view);

        view.title = label(view.container, "---", 8, 4, &lv_font_montserrat_12, kMuted);
        view.value = label(view.container, "---", 8, 22, &lv_font_montserrat_20, kText);
        view.unit = label(view.container, "", 112, 34, &lv_font_montserrat_12, kMuted);
        lv_obj_set_width(view.unit, 56);
        lv_obj_set_style_text_align(view.unit, LV_TEXT_ALIGN_RIGHT, 0);

        view.icon = lv_canvas_create(view.container);
        lv_canvas_set_buffer(view.icon, view.icon_buffer.data(), 16, 16, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_pos(view.icon, 178, 21);
        lv_obj_add_flag(view.icon, LV_OBJ_FLAG_GESTURE_BUBBLE);
    }
}

void Ui::createDash() {
    dash_ = lv_obj_create(nullptr);
    styleScreen(dash_);
    lv_obj_add_event_cb(dash_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    createShiftBar(dash_, dash_shift_);

    lv_obj_t* center = panel(dash_, 224, 42, 352, 156, kPanelAlt);
    label(center, "RPM", 66, 14, &lv_font_montserrat_12, kMuted);
    dash_rpm_ = label(center, "0000", 42, 36, &lv_font_montserrat_48);
    label(center, "GEAR", 241, 14, &lv_font_montserrat_12, kMuted);
    dash_gear_ = label(center, "N", 251, 34, &lv_font_montserrat_48, kYellow);

    lv_obj_t* divider = lv_obj_create(center);
    lv_obj_set_pos(divider, 210, 18);
    lv_obj_set_size(divider, 1, 78);
    lv_obj_set_style_bg_color(divider, kLine, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(divider, LV_OBJ_FLAG_GESTURE_BUBBLE);

    dash_source_ = label(center, "DATA SOURCE --", 12, 120, &lv_font_montserrat_12, kMuted);
    lv_obj_set_width(dash_source_, 328);
    lv_obj_set_style_text_align(dash_source_, LV_TEXT_ALIGN_CENTER, 0);

    createConfigurableTiles(dash_, dash_tiles_, false);
}

void Ui::createTrack() {
    track_ = lv_obj_create(nullptr);
    styleScreen(track_);
    lv_obj_add_event_cb(track_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    createShiftBar(track_, track_shift_);

    lv_obj_t* center = panel(track_, 224, 42, 352, 280, kPanelAlt);
    label(center, "RPM", 64, 12, &lv_font_montserrat_12, kMuted);
    track_rpm_ = label(center, "0000", 40, 31, &lv_font_montserrat_40);
    label(center, "GEAR", 242, 12, &lv_font_montserrat_12, kMuted);
    track_gear_ = label(center, "N", 252, 29, &lv_font_montserrat_40, kYellow);

    lv_obj_t* separator = lv_obj_create(center);
    lv_obj_set_pos(separator, 20, 94);
    lv_obj_set_size(separator, 312, 1);
    lv_obj_set_style_bg_color(separator, kLine, 0);
    lv_obj_set_style_border_width(separator, 0, 0);
    lv_obj_clear_flag(separator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(separator, LV_OBJ_FLAG_GESTURE_BUBBLE);

    label(center, "LAP TIME", 137, 112, &lv_font_montserrat_12, kMuted);
    lv_obj_t* lap = label(center, "--:--.--", 74, 136, &lv_font_montserrat_36, kText);
    lv_obj_set_width(lap, 204);
    lv_obj_set_style_text_align(lap, LV_TEXT_ALIGN_CENTER, 0);
    label(center, "BEST  --:--.--", 94, 202, &lv_font_montserrat_14, kPurple);
    label(center, "DELTA  --.--", 110, 232, &lv_font_montserrat_14, kGreen);

    createConfigurableTiles(track_, track_tiles_, true);
}

void Ui::createDiag() {
    diag_ = lv_obj_create(nullptr);
    styleScreen(diag_);
    lv_obj_add_event_cb(diag_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    label(diag_, "OPENDASH DIAGNOSTICS", 24, 16, &lv_font_montserrat_28);
    diag_source_ = label(diag_, "DATA SOURCE: --", 26, 54, &lv_font_montserrat_12, kMuted);

    lv_obj_t* status_panel = panel(diag_, 24, 88, 240, 132);
    label(status_panel, "DISPLAY / TOUCH", 14, 14, &lv_font_montserrat_12, kBlue);
    diag_status_ = label(status_panel, "DISPLAY --\nTOUCH --\n800 x 480", 14, 42,
                         &lv_font_montserrat_16, kText);

    lv_obj_t* memory_panel = panel(diag_, 280, 88, 240, 132);
    label(memory_panel, "MEMORY", 14, 14, &lv_font_montserrat_12, kBlue);
    diag_memory_ = label(memory_panel, "PSRAM --\nHEAP --", 14, 42,
                         &lv_font_montserrat_16, kText);

    lv_obj_t* runtime_panel = panel(diag_, 536, 88, 240, 132);
    label(runtime_panel, "RUNTIME", 14, 14, &lv_font_montserrat_12, kBlue);
    diag_runtime_ = label(runtime_panel, "UPTIME --\nUI UPDATES --", 14, 42,
                          &lv_font_montserrat_16, kText);

    diag_can_box_ = panel(diag_, 24, 244, 752, 150, lv_color_hex(0x25120F), kRed);
    label(diag_can_box_, "CAN / ECU DATA", 18, 16, &lv_font_montserrat_12, kMuted);
    diag_can_state_ = label(diag_can_box_, "OFFLINE", 18, 46, &lv_font_montserrat_28, kRed);
    diag_can_stats_ = label(diag_can_box_, "1000 kbit/s   RX 0   BAD 0", 322, 54,
                            &lv_font_montserrat_16, kMuted);
}

void Ui::createSettings() {
    settings_ = lv_obj_create(nullptr);
    styleScreen(settings_);
    lv_obj_add_event_cb(settings_, gestureEvent, LV_EVENT_GESTURE, nullptr);

    label(settings_, "OPENDASH SETTINGS", 24, 16, &lv_font_montserrat_28);
    label(settings_, "CONFIGURATION", 26, 54, &lv_font_montserrat_12, kMuted);

    lv_obj_t* body = lv_obj_create(settings_);
    lv_obj_set_pos(body, 24, 82);
    lv_obj_set_size(body, 752, 374);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 0, 0);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_width(body, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(body, kBlue, LV_PART_SCROLLBAR);
    lv_obj_add_flag(body, LV_OBJ_FLAG_GESTURE_BUBBLE);

    for (uint8_t i = 0; i < SettingsScreenModel::sectionCount(); ++i) {
        lv_obj_t* card = lv_obj_create(body);
        lv_obj_set_pos(card, 8, 4 + static_cast<int>(i) * 72);
        lv_obj_set_size(card, 720, 62);
        lv_obj_set_style_bg_color(card, kPanel, 0);
        lv_obj_set_style_border_color(card, kLine, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE);

        lv_obj_t* accent = lv_obj_create(card);
        lv_obj_set_pos(accent, 0, 11);
        lv_obj_set_size(accent, 4, 40);
        lv_obj_set_style_bg_color(accent, i == 0U ? kBlue : kLine, 0);
        lv_obj_set_style_border_width(accent, 0, 0);
        lv_obj_set_style_radius(accent, 2, 0);
        lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(accent, LV_OBJ_FLAG_GESTURE_BUBBLE);

        label(card, SettingsScreenModel::sectionLabel(i), 22, 18, &lv_font_montserrat_20,
              i == 0U ? kBlue : kText);
        label(card, ">", 682, 18, &lv_font_montserrat_20, kMuted);
        settings_overlay_.bindSectionCard(card, i);
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
        const TilePlacement p = TileLayout::placement(slot);
        lv_obj_set_pos(view.container, p.x, p.y);
        lv_obj_set_size(view.container, p.width, p.height);
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
    if (icon == IconId::Invalid || static_cast<uint16_t>(icon) >= static_cast<uint16_t>(IconId::Count)) {
        icon = registry_->descriptor(config.parameter).default_icon;
    }

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
        const lv_color_t accent = tileAccent(tile.parameter);
        const char* title_text = tile.custom_label_enabled && tile.custom_label[0] != '\0'
                                     ? tile.custom_label.data()
                                     : descriptor.short_name;
        lv_label_set_text(view.title, title_text);
        lv_obj_set_style_text_color(view.title, accent, 0);
        lv_obj_set_style_border_color(view.container, accent, 0);

        const ParameterValue& value = registry_->value(tile.parameter);
        if (!value.valid) {
            lv_label_set_text(view.value, "---");
        } else if (descriptor.is_boolean) {
            lv_label_set_text(view.value, value.value >= 0.5f ? "ON" : "OFF");
        } else {
            const float presented = TileEngine::presentValue(tile, value.value, config_->stoich_afr);
            char value_text[32];
            std::snprintf(value_text, sizeof(value_text), "%.*f",
                          static_cast<int>(tile.decimals), static_cast<double>(presented));
            lv_label_set_text(view.value, value_text);
        }

        const char* unit_text = descriptor.is_boolean ? "" : descriptor.unit;
        if ((tile.parameter == ParameterId::Lambda || tile.parameter == ParameterId::LambdaTarget) &&
            tile.value_format == ValueFormatMode::Afr) {
            unit_text = "AFR";
        }
        lv_label_set_text(view.unit, unit_text);
        renderTileIcon(view, tile);
    }
}

void Ui::refreshAllTiles() {
    if (config_ == nullptr) return;
    refreshTileLayout(dash_tiles_, config_->dash_tiles[0]);
    refreshTileLayout(track_tiles_, config_->track_tiles);
    refreshTileValues(dash_tiles_, config_->dash_tiles[0]);
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

    refreshTileValues(dash_tiles_, config_->dash_tiles[0]);
    refreshTileValues(track_tiles_, config_->track_tiles);

    std::snprintf(buf, sizeof(buf), "DISPLAY %s\nTOUCH %s\n800 x 480",
                  d.display_ok ? "OK" : "FAIL", d.touch_ok ? "OK" : "NO");
    lv_label_set_text(diag_status_, buf);
    std::snprintf(buf, sizeof(buf), "PSRAM %.1f MB\nHEAP %u KB",
                  static_cast<double>(d.psram_total) / (1024.0 * 1024.0),
                  static_cast<unsigned>(d.free_heap / 1024U));
    lv_label_set_text(diag_memory_, buf);
    std::snprintf(buf, sizeof(buf), "UPTIME %u s\nUI UPDATES %u",
                  static_cast<unsigned>(d.uptime_ms / 1000U),
                  static_cast<unsigned>(d.ui_updates));
    lv_label_set_text(diag_runtime_, buf);
    std::snprintf(buf, sizeof(buf), "DATA SOURCE: %s", dataSourceName(telemetry.source));
    lv_label_set_text(diag_source_, buf);

    const bool online = telemetry.can_driver_running && telemetry.can_online;
    const bool driver_ready = telemetry.can_driver_running;
    lv_label_set_text(diag_can_state_, online ? "ONLINE" : (driver_ready ? "WAITING FOR ECU" : "DRIVER ERROR"));
    const lv_color_t can_color = online ? kGreen : (driver_ready ? kYellow : kRed);
    lv_obj_set_style_text_color(diag_can_state_, can_color, 0);
    lv_obj_set_style_border_color(diag_can_box_, can_color, 0);
    lv_obj_set_style_bg_color(diag_can_box_, online ? lv_color_hex(0x0D2416) : lv_color_hex(0x25120F), 0);

    std::snprintf(buf, sizeof(buf), "%u kbit/s   RX %u   BAD %u",
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

void Ui::layoutSettingsHandler(void* context, bool track) {
    auto* self = static_cast<Ui*>(context);
    if (self == nullptr) return;
    self->openTileEditor(track ? self->track_tiles_[0] : self->dash_tiles_[0]);
}

void Ui::settingsChangedHandler(void* context) {
    auto* self = static_cast<Ui*>(context);
    if (self != nullptr) self->refreshAllTiles();
}

void Ui::openTileEditor(TileView& view) {
    settings_overlay_.close();
    closeTileEditor();
    editing_tile_ = &view;

    tile_editor_overlay_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(tile_editor_overlay_, 620, 330);
    lv_obj_center(tile_editor_overlay_);
    lv_obj_set_style_bg_color(tile_editor_overlay_, lv_color_hex(0x0C1117), 0);
    lv_obj_set_style_bg_opa(tile_editor_overlay_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tile_editor_overlay_, kBlue, 0);
    lv_obj_set_style_border_width(tile_editor_overlay_, 2, 0);
    lv_obj_set_style_radius(tile_editor_overlay_, 12, 0);
    lv_obj_clear_flag(tile_editor_overlay_, LV_OBJ_FLAG_SCROLLABLE);

    auto& profile = view.track ? config_->track_tiles : config_->dash_tiles[0];
    TileConfig& tile = profile[view.config_index];
    const ParameterDescriptor& descriptor = registry_->descriptor(tile.parameter);

    char title_text[64];
    std::snprintf(title_text, sizeof(title_text), "%s TILE %u",
                  view.track ? "TRACK" : "DASH",
                  static_cast<unsigned>(view.config_index + 1U));
    label(tile_editor_overlay_, title_text, 22, 14, &lv_font_montserrat_24);
    label(tile_editor_overlay_, descriptor.name, 24, 52, &lv_font_montserrat_18, kBlue);

    char state_text[128];
    const char* icon_text = !tile.icon_enabled ? "OFF" :
                            (tile.icon == 0U ? "DEFAULT" : IconCatalog::name(static_cast<IconId>(tile.icon)));
    std::snprintf(state_text, sizeof(state_text), "%s  |  ICON %s  |  %u DEC  |  %s",
                  tile.visible ? "VISIBLE" : "HIDDEN", icon_text,
                  static_cast<unsigned>(tile.decimals),
                  tile.value_format == ValueFormatMode::Afr ? "AFR" : "NATIVE");
    label(tile_editor_overlay_, state_text, 24, 80, &lv_font_montserrat_12, kMuted);

    actionButton(tile_editor_overlay_, "< PARAM", 24, 116, 112, lv_color_hex(0x153B57), [](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        TileView* view_ptr = ui->editing_tile_;
        auto& profile_ref = view_ptr->track ? ui->config_->track_tiles : ui->config_->dash_tiles[0];
        TileConfig& cfg = profile_ref[view_ptr->config_index];
        TileEditorModel::setParameter(cfg, ParameterPickerModel::previousVisible(*ui->config_, cfg.parameter));
        ui->saveConfig();
        ui->openTileEditor(*view_ptr);
    });

    actionButton(tile_editor_overlay_, "PARAM >", 144, 116, 112, lv_color_hex(0x153B57), [](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        TileView* view_ptr = ui->editing_tile_;
        auto& profile_ref = view_ptr->track ? ui->config_->track_tiles : ui->config_->dash_tiles[0];
        TileConfig& cfg = profile_ref[view_ptr->config_index];
        TileEditorModel::setParameter(cfg, ParameterPickerModel::nextVisible(*ui->config_, cfg.parameter));
        ui->saveConfig();
        ui->openTileEditor(*view_ptr);
    });

    actionButton(tile_editor_overlay_, tile.visible ? "HIDE TILE" : "SHOW TILE", 264, 116, 112,
                 tile.visible ? lv_color_hex(0x6B321D) : lv_color_hex(0x15592A), [](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        TileView* view_ptr = ui->editing_tile_;
        auto& profile_ref = view_ptr->track ? ui->config_->track_tiles : ui->config_->dash_tiles[0];
        TileConfig& cfg = profile_ref[view_ptr->config_index];
        TileEditorModel::setVisible(cfg, !cfg.visible);
        ui->saveConfig();
        ui->openTileEditor(*view_ptr);
    });

    actionButton(tile_editor_overlay_, "ICON >", 384, 116, 96, lv_color_hex(0x3C285C), [](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        TileView* view_ptr = ui->editing_tile_;
        auto& profile_ref = view_ptr->track ? ui->config_->track_tiles : ui->config_->dash_tiles[0];
        TileConfig& cfg = profile_ref[view_ptr->config_index];
        const uint16_t last_icon = static_cast<uint16_t>(IconId::Count) - 1U;
        if (!cfg.icon_enabled) {
            TileEditorModel::useDefaultIcon(cfg);
        } else if (cfg.icon == 0U || cfg.icon >= last_icon) {
            TileEditorModel::useCustomIcon(cfg, IconId::Generic);
        } else {
            TileEditorModel::useCustomIcon(cfg, static_cast<IconId>(cfg.icon + 1U));
        }
        ui->saveConfig();
        ui->openTileEditor(*view_ptr);
    });

    actionButton(tile_editor_overlay_, "DEC +", 488, 116, 104, lv_color_hex(0x254254), [](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        TileView* view_ptr = ui->editing_tile_;
        auto& profile_ref = view_ptr->track ? ui->config_->track_tiles : ui->config_->dash_tiles[0];
        TileConfig& cfg = profile_ref[view_ptr->config_index];
        cfg.decimals = static_cast<uint8_t>((cfg.decimals + 1U) % 4U);
        ui->saveConfig();
        ui->openTileEditor(*view_ptr);
    });

    const bool lambda_parameter = tile.parameter == ParameterId::Lambda || tile.parameter == ParameterId::LambdaTarget;
    actionButton(tile_editor_overlay_, lambda_parameter ?
                     (tile.value_format == ValueFormatMode::Afr ? "SHOW LAMBDA" : "SHOW AFR") : "AFR N/A",
                 24, 170, 152, lambda_parameter ? lv_color_hex(0x56327A) : lv_color_hex(0x292D33),
                 lambda_parameter ? +[](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        TileView* view_ptr = ui->editing_tile_;
        auto& profile_ref = view_ptr->track ? ui->config_->track_tiles : ui->config_->dash_tiles[0];
        TileConfig& cfg = profile_ref[view_ptr->config_index];
        TileEditorModel::setAfrMode(cfg, cfg.value_format != ValueFormatMode::Afr);
        ui->saveConfig();
        ui->openTileEditor(*view_ptr);
    } : nullptr);

    actionButton(tile_editor_overlay_, "< TILE", 192, 170, 112, lv_color_hex(0x153B57), [](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        const bool track = ui->editing_tile_->track;
        const uint8_t slot = TileSlotNavigation::previous(ui->editing_tile_->config_index);
        TileView& target = track ? ui->track_tiles_[slot] : ui->dash_tiles_[slot];
        ui->openTileEditor(target);
    });

    actionButton(tile_editor_overlay_, "TILE >", 312, 170, 112, lv_color_hex(0x153B57), [](lv_event_t*) {
        Ui* ui = Ui::instance_;
        if (ui == nullptr || ui->editing_tile_ == nullptr) return;
        const bool track = ui->editing_tile_->track;
        const uint8_t slot = TileSlotNavigation::next(ui->editing_tile_->config_index);
        TileView& target = track ? ui->track_tiles_[slot] : ui->dash_tiles_[slot];
        ui->openTileEditor(target);
    });

    actionButton(tile_editor_overlay_, "CLOSE", 448, 258, 144, lv_color_hex(0x153B57), [](lv_event_t*) {
        if (Ui::instance_ != nullptr) Ui::instance_->closeTileEditor();
    });
}

void Ui::closeTileEditor() {
    if (tile_editor_overlay_ != nullptr) {
        lv_obj_t* stale_overlay = tile_editor_overlay_;
        tile_editor_overlay_ = nullptr;
        lv_obj_del_async(stale_overlay);
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
        instance_->tile_editor_overlay_ != nullptr || instance_->settings_overlay_.active()) return;
    lv_indev_t* indev = lv_indev_get_act();
    if (indev == nullptr) return;
    const GestureDirection direction = gestureFromLvgl(lv_indev_get_gesture_dir(indev));
    const UiPage next = GestureNavigation::navigate(instance_->current_page_, direction);
    if (next != instance_->current_page_) instance_->load(next);
}

void Ui::load(UiPage page) {
    settings_overlay_.close();
    closeTileEditor();
    current_page_ = page;
    switch (page) {
        case UiPage::Dash: lv_scr_load(dash_); break;
        case UiPage::Track: lv_scr_load(track_); break;
        case UiPage::Diag: lv_scr_load(diag_); break;
        case UiPage::Settings: lv_scr_load(settings_); break;
    }
}

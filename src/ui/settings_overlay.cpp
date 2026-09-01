#include "settings_overlay.h"

#include <cstdio>

#include "ui/can_settings_model.h"
#include "ui/parameter_picker_model.h"

namespace {
const lv_color_t kPanel = lv_color_hex(0x0C1117);
const lv_color_t kText = lv_color_hex(0xF2F5F7);
const lv_color_t kMuted = lv_color_hex(0x7D8996);
const lv_color_t kBlue = lv_color_hex(0x20A4F3);
const lv_color_t kGreen = lv_color_hex(0x39D353);
const lv_color_t kRed = lv_color_hex(0xFF3B30);

const char* sourceName(DataSource source) {
    switch (source) {
        case DataSource::Mock: return "MOCK";
        case DataSource::Ecumaster: return "ECUMASTER";
        case DataSource::Rusefi: return "RUSEFI";
    }
    return "UNKNOWN";
}
}

void SettingsOverlay::begin(AppConfig& config, ParameterRegistry& registry, NvsConfigStore& store,
                            void* context, LayoutHandler layout_handler,
                            ChangedHandler changed_handler) {
    config_ = &config;
    registry_ = &registry;
    store_ = &store;
    context_ = context;
    layout_handler_ = layout_handler;
    changed_handler_ = changed_handler;

    for (uint8_t i = 0; i < 7U; ++i) {
        card_contexts_[i].owner = this;
        card_contexts_[i].section = i;
    }
}

void SettingsOverlay::bindSectionCard(lv_obj_t* card, uint8_t section_index) {
    if (card == nullptr || section_index >= 7U) return;
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, cardEvent, LV_EVENT_CLICKED, &card_contexts_[section_index]);
}

void SettingsOverlay::cardEvent(lv_event_t* event) {
    auto* ctx = static_cast<CardContext*>(lv_event_get_user_data(event));
    if (ctx != nullptr && ctx->owner != nullptr) ctx->owner->openSection(ctx->section);
}

void SettingsOverlay::openSection(uint8_t section) {
    switch (section) {
        case 0: openCan(); break;
        case 1:
            close();
            if (layout_handler_ != nullptr) layout_handler_(context_, false);
            break;
        case 2:
            close();
            if (layout_handler_ != nullptr) layout_handler_(context_, true);
            break;
        case 3: openParameters(); break;
        case 4: openInfo("WARNINGS", "Warning editor UI is the next Task 9 step.\nRuntime warning engine is already active."); break;
        case 5: openFuel(); break;
        case 6: openSystem(); break;
        default: break;
    }
}

lv_obj_t* SettingsOverlay::createOverlay(const char* title) {
    close();
    overlay_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay_, 650, 350);
    lv_obj_center(overlay_);
    lv_obj_set_style_bg_color(overlay_, kPanel, 0);
    lv_obj_set_style_bg_opa(overlay_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(overlay_, kBlue, 0);
    lv_obj_set_style_border_width(overlay_, 2, 0);
    lv_obj_set_style_radius(overlay_, 12, 0);
    lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_SCROLLABLE);
    text(title, 22, 14, &lv_font_montserrat_24, kText);
    button("CLOSE", 506, 288, 116, closeEvent);
    return overlay_;
}

lv_obj_t* SettingsOverlay::text(const char* value, int x, int y, const lv_font_t* font,
                                lv_color_t color) {
    if (overlay_ == nullptr) return nullptr;
    lv_obj_t* obj = lv_label_create(overlay_);
    lv_label_set_text(obj, value);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color, 0);
    return obj;
}

lv_obj_t* SettingsOverlay::button(const char* value, int x, int y, int w,
                                  lv_event_cb_t callback) {
    if (overlay_ == nullptr) return nullptr;
    lv_obj_t* btn = lv_btn_create(overlay_);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, 42);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x153B57), 0);
    lv_obj_set_style_radius(btn, 7, 0);
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, this);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, value);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
    return btn;
}

void SettingsOverlay::closeEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self != nullptr) self->close();
}

void SettingsOverlay::close() {
    if (overlay_ != nullptr) {
        // Settings callbacks frequently rebuild the same modal after changing a
        // value. Deleting the modal synchronously from one of its own LVGL event
        // callbacks invalidates the event target while LVGL is still walking the
        // event stack, which can leave duplicated/corrupted top-layer objects.
        // Detach our pointer immediately but let LVGL delete the object on its
        // next async pass, after the current callback has returned.
        lv_obj_t* stale_overlay = overlay_;
        overlay_ = nullptr;
        lv_obj_del_async(stale_overlay);
    }
}

void SettingsOverlay::openCan() {
    if (config_ == nullptr) return;
    createOverlay("CAN SETTINGS");

    char line[96];
    std::snprintf(line, sizeof(line), "SOURCE: %s", sourceName(config_->data_source));
    text(line, 26, 58, &lv_font_montserrat_18, kBlue);
    std::snprintf(line, sizeof(line), "BITRATE: %u kbit/s", static_cast<unsigned>(config_->can_bitrate / 1000U));
    text(line, 26, 88, &lv_font_montserrat_18, kText);
    std::snprintf(line, sizeof(line), "EMU BASE: 0x%03X", static_cast<unsigned>(config_->ecumaster_base_id));
    text(line, 26, 118, &lv_font_montserrat_18, kText);
    std::snprintf(line, sizeof(line), "TIMEOUT: %u ms", static_cast<unsigned>(config_->can_timeout_ms));
    text(line, 26, 148, &lv_font_montserrat_18, kText);
    text("Source/bitrate changes apply after restart.", 26, 184, &lv_font_montserrat_12, kMuted);

    button("SOURCE >", 330, 58, 130, canSourceEvent);
    button("BITRATE >", 472, 58, 130, canBitrateEvent);
    button("BASE -", 330, 116, 90, canBaseDownEvent);
    button("BASE +", 432, 116, 90, canBaseUpEvent);
    button("TIME -", 330, 174, 90, canTimeoutDownEvent);
    button("TIME +", 432, 174, 90, canTimeoutUpEvent);
}

void SettingsOverlay::canSourceEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    CanSettingsModel::nextSource(*self->config_);
    self->save();
    self->openCan();
}

void SettingsOverlay::canBitrateEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    CanSettingsModel::nextBitrate(*self->config_);
    self->save();
    self->openCan();
}

void SettingsOverlay::canBaseDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    CanSettingsModel::adjustBaseId(*self->config_, -0x10);
    self->save();
    self->openCan();
}

void SettingsOverlay::canBaseUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    CanSettingsModel::adjustBaseId(*self->config_, 0x10);
    self->save();
    self->openCan();
}

void SettingsOverlay::canTimeoutDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    CanSettingsModel::adjustTimeout(*self->config_, -100);
    self->save();
    self->openCan();
}

void SettingsOverlay::canTimeoutUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    CanSettingsModel::adjustTimeout(*self->config_, 100);
    self->save();
    self->openCan();
}

void SettingsOverlay::openParameters() {
    if (config_ == nullptr || registry_ == nullptr) return;
    createOverlay("PARAMETERS");
    if (selected_parameter_ >= AppConfig::kParameterCount) selected_parameter_ = 0U;

    const ParameterId id = static_cast<ParameterId>(selected_parameter_);
    const ParameterDescriptor& descriptor = registry_->descriptor(id);
    text(descriptor.name, 26, 62, &lv_font_montserrat_20, kBlue);

    char line[96];
    std::snprintf(line, sizeof(line), "ID %u / %u   %s", static_cast<unsigned>(selected_parameter_ + 1U),
                  static_cast<unsigned>(AppConfig::kParameterCount),
                  config_->parameter_visible[selected_parameter_] ? "VISIBLE IN PICKER" : "HIDDEN FROM PICKER");
    text(line, 26, 96, &lv_font_montserrat_14,
         config_->parameter_visible[selected_parameter_] ? kGreen : kRed);
    text("Hiding here does not disable decoding or warnings.", 26, 132, &lv_font_montserrat_12, kMuted);

    button("< PREV", 26, 184, 120, parameterPrevEvent);
    button("NEXT >", 158, 184, 120, parameterNextEvent);
    button(config_->parameter_visible[selected_parameter_] ? "HIDE" : "SHOW",
           290, 184, 120, parameterToggleEvent);
}

void SettingsOverlay::parameterPrevEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->selected_parameter_ = self->selected_parameter_ == 0U
                                    ? AppConfig::kParameterCount - 1U
                                    : self->selected_parameter_ - 1U;
    self->openParameters();
}

void SettingsOverlay::parameterNextEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->selected_parameter_ = static_cast<uint16_t>((self->selected_parameter_ + 1U) % AppConfig::kParameterCount);
    self->openParameters();
}

void SettingsOverlay::parameterToggleEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    const ParameterId id = static_cast<ParameterId>(self->selected_parameter_);
    const bool new_state = !self->config_->parameter_visible[self->selected_parameter_];
    ParameterPickerModel::setVisible(*self->config_, id, new_state);
    self->refreshRegistryVisibility();
    self->save();
    self->openParameters();
}

void SettingsOverlay::openFuel() {
    if (config_ == nullptr) return;
    createOverlay("FUEL / AFR");

    char line[96];
    std::snprintf(line, sizeof(line), "STOICH AFR: %.1f", static_cast<double>(config_->stoich_afr));
    text(line, 26, 70, &lv_font_montserrat_24, kBlue);
    text(config_->lambda_format == ValueFormatMode::Afr ? "GLOBAL DEFAULT: AFR" : "GLOBAL DEFAULT: LAMBDA",
         26, 112, &lv_font_montserrat_18, kText);
    text("AFR = Lambda x Stoich AFR. Individual tiles may override format.", 26, 154,
         &lv_font_montserrat_12, kMuted);

    button("STOICH -", 26, 202, 130, stoichDownEvent);
    button("STOICH +", 168, 202, 130, stoichUpEvent);
    button("LAMBDA / AFR", 310, 202, 170, lambdaFormatEvent);
}

void SettingsOverlay::stoichDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    self->config_->stoich_afr -= 0.1f;
    if (self->config_->stoich_afr < 5.0f) self->config_->stoich_afr = 5.0f;
    self->save();
    self->openFuel();
}

void SettingsOverlay::stoichUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    self->config_->stoich_afr += 0.1f;
    if (self->config_->stoich_afr > 20.0f) self->config_->stoich_afr = 20.0f;
    self->save();
    self->openFuel();
}

void SettingsOverlay::lambdaFormatEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    self->config_->lambda_format = self->config_->lambda_format == ValueFormatMode::Native
                                       ? ValueFormatMode::Afr
                                       : ValueFormatMode::Native;
    self->applyGlobalLambdaFormat();
    self->save();
    self->openFuel();
}

void SettingsOverlay::applyGlobalLambdaFormat() {
    if (config_ == nullptr) return;
    const auto apply = [this](auto& tiles) {
        for (auto& tile : tiles) {
            if (tile.parameter == ParameterId::Lambda || tile.parameter == ParameterId::LambdaTarget) {
                tile.value_format = config_->lambda_format;
            }
        }
    };
    apply(config_->tiles);
    apply(config_->track_tiles);
}

void SettingsOverlay::openSystem() {
    if (config_ == nullptr) return;
    createOverlay("SYSTEM");
    char line[96];
    std::snprintf(line, sizeof(line), "CONFIG SCHEMA: v%u", static_cast<unsigned>(config_->schema_version));
    text(line, 26, 72, &lv_font_montserrat_18, kText);
    text("Factory reset restores CAN, layouts, parameters, warnings and AFR.", 26, 112,
         &lv_font_montserrat_12, kMuted);
    button("FACTORY RESET", 26, 170, 180, factoryResetEvent);
}

void SettingsOverlay::factoryResetEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr || self->config_ == nullptr) return;
    *self->config_ = AppConfig::defaults();
    self->refreshRegistryVisibility();
    self->save();
    self->openInfo("SYSTEM", "Factory defaults saved.\nRestart OpenDash to reinitialize CAN hardware.");
}

void SettingsOverlay::openInfo(const char* title, const char* message) {
    createOverlay(title);
    text(message, 26, 76, &lv_font_montserrat_16, kMuted);
}

void SettingsOverlay::refreshRegistryVisibility() {
    if (config_ == nullptr || registry_ == nullptr) return;
    for (uint16_t i = 0; i < AppConfig::kParameterCount; ++i) {
        registry_->setPickerVisible(static_cast<ParameterId>(i), config_->parameter_visible[i]);
    }
}

void SettingsOverlay::save() {
    if (config_ == nullptr || store_ == nullptr) return;
    config_->validate();
    store_->save(*config_);
    notifyChanged();
}

void SettingsOverlay::notifyChanged() {
    if (changed_handler_ != nullptr) changed_handler_(context_);
}

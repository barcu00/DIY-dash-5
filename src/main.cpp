#include <Arduino.h>
#include <array>
#include <lvgl.h>

#include "board/board_display.h"
#include "settings/app_config.h"
#include "telemetry/parameter_registry.h"
#include "ui/icon_assets.h"

namespace {
BoardDisplay board;
lv_obj_t* screen = nullptr;
lv_obj_t* stage_label = nullptr;
std::array<lv_obj_t*, AppConfig::kTileCount * 2U> canvases{};
std::array<std::array<lv_color_t, 16U * 16U>, AppConfig::kTileCount * 2U> buffers{};
ParameterRegistry* registry = nullptr;
AppConfig config{};
uint8_t stage = 0;
uint32_t next_stage_ms = 0;

void setStage(const char* text, lv_color_t color) {
    if (stage_label == nullptr) return;
    lv_label_set_text(stage_label, text);
    lv_obj_set_style_text_color(stage_label, color, 0);
}

void createBase() {
    screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x072812), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "OPENDASH REGISTRY / ICON TEST");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    stage_label = lv_label_create(screen);
    lv_label_set_text(stage_label, "STAGE 1 / DISPLAY OK");
    lv_obj_set_style_text_font(stage_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(stage_label, lv_color_hex(0x39D353), 0);
    lv_obj_align(stage_label, LV_ALIGN_TOP_MID, 0, 70);

    lv_scr_load(screen);
}

void createRegistryAndConfig() {
    registry = new ParameterRegistry();
    config = AppConfig::defaults();
    config.validate();
    setStage("STAGE 2 / REGISTRY + CONFIG OK", lv_color_hex(0x20A4F3));
}

void createRealCanvases() {
    setStage("STAGE 3 / 24 REAL TILE CANVASES", lv_color_hex(0xB26BFF));
    for (uint8_t i = 0; i < AppConfig::kTileCount * 2U; ++i) {
        lv_obj_t* tile = lv_obj_create(screen);
        const uint8_t col = static_cast<uint8_t>(i % 6U);
        const uint8_t row = static_cast<uint8_t>((i / 6U) % 4U);
        lv_obj_set_pos(tile, 10 + col * 130, 120 + row * 80);
        lv_obj_set_size(tile, 120, 70);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x11161D), 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

        canvases[i] = lv_canvas_create(tile);
        lv_canvas_set_buffer(canvases[i], buffers[i].data(), 16, 16, LV_IMG_CF_TRUE_COLOR);
        lv_obj_center(canvases[i]);
        lv_canvas_fill_bg(canvases[i], lv_color_hex(0x11161D), LV_OPA_COVER);
    }
}

void renderActualIcons() {
    setStage("STAGE 4 / RENDERING REAL ICON ASSETS", lv_color_hex(0xFFCF33));
    for (uint8_t i = 0; i < AppConfig::kTileCount * 2U; ++i) {
        const TileConfig& tile = i < AppConfig::kTileCount
                                     ? config.tiles[i]
                                     : config.track_tiles[i - AppConfig::kTileCount];
        const IconId icon = tile.icon == 0U
                                ? registry->descriptor(tile.parameter).default_icon
                                : static_cast<IconId>(tile.icon);
        const IconAsset& asset = IconAssets::get(icon);
        if (asset.rows == nullptr || asset.width != 16U || asset.height != 16U) continue;

        lv_canvas_fill_bg(canvases[i], lv_color_hex(0x11161D), LV_OPA_COVER);
        for (uint8_t y = 0; y < 16U; ++y) {
            const uint16_t bits = asset.rows[y];
            for (uint8_t x = 0; x < 16U; ++x) {
                if ((bits & static_cast<uint16_t>(1U << (15U - x))) != 0U) {
                    lv_canvas_set_px_color(canvases[i], x, y, lv_color_hex(0x20A4F3));
                }
            }
        }
    }
}

void validateDescriptors() {
    for (uint16_t i = 0; i < ParameterRegistry::count(); ++i) {
        const ParameterDescriptor& descriptor = registry->descriptor(static_cast<ParameterId>(i));
        volatile const char* name = descriptor.name;
        volatile IconId icon = descriptor.default_icon;
        (void)name;
        (void)icon;
    }
    setStage("STAGE 5 / ALL 90 DESCRIPTORS OK", lv_color_hex(0x39D353));
}

void complete() {
    setStage("STAGE 6 / REGISTRY + ICON PATH SURVIVED", lv_color_hex(0x39D353));
}

void nextStage() {
    if (!board.lock()) return;
    ++stage;
    switch (stage) {
        case 2: createRegistryAndConfig(); break;
        case 3: createRealCanvases(); break;
        case 4: renderActualIcons(); break;
        case 5: validateDescriptors(); break;
        case 6: complete(); break;
        default: break;
    }
    board.unlock();
}
}

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println("[OpenDash] registry/icon staged self-test");
    if (!board.begin()) return;
    if (board.lock()) {
        createBase();
        board.unlock();
    }
    stage = 1;
    next_stage_ms = millis() + 1800U;
}

void loop() {
    board.service();
    if (stage >= 1U && stage < 6U && static_cast<int32_t>(millis() - next_stage_ms) >= 0) {
        nextStage();
        next_stage_ms = millis() + 1800U;
    }
    delay(2);
}

#include <unity.h>
#include <cmath>
#include "ui/dashboard_layout.h"
#include "ui/tile_engine.h"
#include "ui/tile_editor_model.h"
#include "alarms/tile_warning_engine.h"

void test_analog_center_is_an_independent_editable_tile_and_can_be_hidden() {
    AppConfig config=AppConfig::defaults();
    config.dash_layout=config.track_layout=DashboardLayout::AnalogStyle;
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash,6},config));
    editor.setParameter(ParameterId::OilPressure);
    editor.setWarning({true,WarningDirection::Below,2.5f,.2f,0});
    TEST_ASSERT_TRUE(editor.applyTo(config));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Rpm),static_cast<uint8_t>(activeTiles(config,PageId::Track)[6].parameter));
    auto placements=TileEngine::placements(PageId::Dash,config);
    TEST_ASSERT_EQUAL_UINT32(7,placements.count);
    bool found=false;
    for(std::size_t i=0;i<placements.count;++i)if(placements.items[i].address.slot==6) {
        TEST_ASSERT_EQUAL_INT16(76,placements.items[i].geometry.x);
        TEST_ASSERT_EQUAL_INT16(156,placements.items[i].geometry.y);
        found=true;
    }
    TEST_ASSERT_TRUE(found);
    VehicleState state;state.set(ParameterId::OilPressure,1.f,100);
    TileWarningEngine warnings;warnings.evaluate(config,state,100);
    TEST_ASSERT_TRUE(warnings.isHighlighted({PageId::Dash,6}));
    activeTiles(config,PageId::Dash)[6].visible=false;
    TEST_ASSERT_EQUAL_UINT32(6,TileEngine::placements(PageId::Dash,config).count);
}

// Catches page selections sharing storage, presets losing saved tile state,
// geometry overflowing the screen, scale coupling to shift thresholds, and
// dormant presets incorrectly raising warnings.
void test_each_page_can_assign_any_preset_and_keep_its_tile_bank() {
    AppConfig config = AppConfig::defaults();
    const std::size_t counts[] = {14, 12, 7, 8, 8};
    for (std::size_t i = 0; i < 5; ++i) {
        auto layout = static_cast<DashboardLayout>(i);
        config.dash_layout = layout;
        config.track_layout = layout;
        auto dash = activeTiles(config, PageId::Dash);
        auto track = activeTiles(config, PageId::Track);
        TEST_ASSERT_EQUAL_UINT32(counts[i], dash.size());
        TEST_ASSERT_EQUAL_UINT32(counts[i], track.size());
        TEST_ASSERT_NOT_EQUAL(dash.data, track.data);
        dash[0].visible = false;
        dash[0].warning = {true, WarningDirection::Below, 2.5f, 0.5f, 100};
        dash[0].flag_active_color = FlagActiveColor::Red;
        config.dash_layout = DashboardLayout::StripStyle;
        config.dash_layout = layout;
        TEST_ASSERT_FALSE(activeTiles(config, PageId::Dash)[0].visible);
        TEST_ASSERT_TRUE(activeTiles(config, PageId::Dash)[0].warning.enabled);
        TEST_ASSERT_TRUE(track[0].visible);
    }
}

void test_all_five_presets_fit_both_pages_without_overlapping_navigation() {
    AppConfig config = AppConfig::defaults();
    for (std::size_t i = 0; i < 5; ++i) {
        config.dash_layout = config.track_layout = static_cast<DashboardLayout>(i);
        for (auto page : {PageId::Dash, PageId::Track}) {
            auto placements = TileEngine::placements(page, config);
            TEST_ASSERT_EQUAL_UINT32(dashboardLayoutSlotCount(static_cast<DashboardLayout>(i)),
                                    placements.count);
            for (std::size_t j = 0; j < placements.count; ++j) {
                const auto& g = placements.items[j].geometry;
                TEST_ASSERT_TRUE(g.x >= 0 && g.y >= 0);
                TEST_ASSERT_TRUE(g.width > 0 && g.height > 0);
                TEST_ASSERT_TRUE(g.x + g.width <= 800);
                TEST_ASSERT_TRUE(g.y + g.height <= 422);
                for (std::size_t k = 0; k < j; ++k) {
                    const auto& h = placements.items[k].geometry;
                    TEST_ASSERT_TRUE(g.x + g.width <= h.x || h.x + h.width <= g.x ||
                        g.y + g.height <= h.y || h.y + h.height <= g.y);
                }
            }
        }
    }
}

void test_shared_scale_clamps_snaps_and_never_changes_shift_thresholds() {
    AppConfig config = AppConfig::defaults();
    auto shift = config.shift;
    config.rpm_scale_max = 6051U;
    TEST_ASSERT_TRUE(config.validate().valid);
    TEST_ASSERT_EQUAL_UINT16(6100U, config.rpm_scale_max);
    TEST_ASSERT_EQUAL_UINT16(shift.flash_rpm, config.shift.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(shift.max_rpm, config.shift.max_rpm);
    config.rpm_scale_max = 0U;
    TEST_ASSERT_TRUE(config.validate().valid);
    TEST_ASSERT_EQUAL_UINT16(100U, config.rpm_scale_max);
    config.rpm_scale_max = 65535U;
    TEST_ASSERT_TRUE(config.validate().valid);
    TEST_ASSERT_EQUAL_UINT16(10000U, config.rpm_scale_max);
    TEST_ASSERT_EQUAL_UINT16(500U, rpmScaleFill(3000, true, 6000));
    TEST_ASSERT_EQUAL_UINT16(1000U, rpmScaleFill(11000, true, 10000));
    TEST_ASSERT_EQUAL_UINT16(0U, rpmScaleFill(-1, true, 10000));
    TEST_ASSERT_EQUAL_UINT16(0U, rpmScaleFill(NAN, true, 10000));
    TEST_ASSERT_EQUAL_UINT16(0U, rpmScaleFill(3000, false, 10000));
    TEST_ASSERT_EQUAL_UINT16(0U, rpmScaleFill(3000, true, 0));
}

void test_invalid_layouts_restore_original_page_choices() {
    AppConfig config = AppConfig::defaults();
    config.dash_layout = config.track_layout = static_cast<DashboardLayout>(255);
    TEST_ASSERT_TRUE(config.validate().valid);
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(config.dash_layout));
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(config.track_layout));
    TEST_ASSERT_EQUAL_UINT32(0, layoutTiles(config, PageId::Settings,
        DashboardLayout::SideGear).size());
}

void test_editor_and_warnings_use_only_the_selected_bank() {
    AppConfig config = AppConfig::defaults();
    config.dash_layout = DashboardLayout::AnalogStyle;
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 1}, config));
    editor.setParameter(ParameterId::OilPressure);
    editor.setWarning({true, WarningDirection::Below, 2.5f, 0.2f, 0});
    TEST_ASSERT_TRUE(editor.applyTo(config));
    TEST_ASSERT_TRUE(activeTiles(config, PageId::Dash)[1].warning.enabled);
    TEST_ASSERT_FALSE(config.dash_tiles[1].warning.enabled);
    TEST_ASSERT_FALSE(editor.open({PageId::Dash, 7}, config));
    VehicleState state;
    state.set(ParameterId::OilPressure, 1.0f, 100);
    TileWarningEngine warnings;
    warnings.evaluate(config, state, 100);
    TEST_ASSERT_TRUE(warnings.nextModal().has_value());
    config.dash_layout = DashboardLayout::SideGear;
    warnings.evaluate(config, state, 120);
    TEST_ASSERT_FALSE(warnings.nextModal().has_value());
    config.dash_layout = DashboardLayout::AnalogStyle;
    warnings.evaluate(config, state, 140);
    TEST_ASSERT_TRUE(warnings.nextModal().has_value());
}

void test_hidden_strip_column_tiles_pack_down_without_moving_center() {
    AppConfig config = AppConfig::defaults();
    config.dash_layout = DashboardLayout::StripStyle;
    activeTiles(config, PageId::Dash)[0].visible = false;
    auto placements = TileEngine::placements(PageId::Dash, config);
    TEST_ASSERT_EQUAL_UINT32(7, placements.count);
    for (std::size_t i = 0; i < placements.count; ++i) {
        auto p = placements.items[i];
        if (p.address.slot == 1) TEST_ASSERT_EQUAL_INT16(262, p.geometry.y);
        if (p.address.slot == 2) TEST_ASSERT_EQUAL_INT16(346, p.geometry.y);
        if (p.address.slot == 3) TEST_ASSERT_EQUAL_INT16(178, p.geometry.y);
    }
}

void test_approved_style_placements_preserve_touch_targets_and_frame_spacing() {
    AppConfig config = AppConfig::defaults();
    struct Fixture { DashboardLayout layout; uint8_t slot; int x,y,w,h; };
    const Fixture fixtures[] = {
        {DashboardLayout::AnalogStyle,0,464,16,328,62},
        {DashboardLayout::AnalogStyle,5,464,356,328,62},
        {DashboardLayout::SideGear,0,8,16,136,292},
        {DashboardLayout::SideGear,1,170,145,292,155},
        {DashboardLayout::SideGear,3,8,316,150,106},
        {DashboardLayout::StripStyle,0,8,178,158,76},
        {DashboardLayout::StripStyle,3,176,178,218,244},
    };
    for (const auto& fixture : fixtures) {
        config.dash_layout = config.track_layout = fixture.layout;
        for (auto page : {PageId::Dash,PageId::Track}) {
            auto placements = TileEngine::placements(page,config);
            bool found=false;
            for (std::size_t i=0;i<placements.count;i++) {
                if (placements.items[i].address.slot != fixture.slot) continue;
                const auto& g=placements.items[i].geometry;
                TEST_ASSERT_EQUAL_INT16(fixture.x,g.x);
                TEST_ASSERT_EQUAL_INT16(fixture.y,g.y);
                TEST_ASSERT_EQUAL_INT16(fixture.w,g.width);
                TEST_ASSERT_EQUAL_INT16(fixture.h,g.height);
                found=true;
            }
            TEST_ASSERT_TRUE(found);
        }
    }
}

void test_sixth_layout_has_independent_editable_rows_and_packs_hidden_tiles_down() {
    const auto layout=static_cast<DashboardLayout>(5);
    TEST_ASSERT_TRUE(validDashboardLayout(layout));
    AppConfig config=AppConfig::defaults();
    config.dash_layout=config.track_layout=layout;
    auto dash=activeTiles(config,PageId::Dash),track=activeTiles(config,PageId::Track);
    TEST_ASSERT_EQUAL_UINT32(6,dash.size());
    TEST_ASSERT_EQUAL_UINT32(6,track.size());
    TEST_ASSERT_NOT_EQUAL(dash.data,track.data);
    TEST_ASSERT_NOT_EQUAL(dash.data,layoutTiles(config,PageId::Dash,DashboardLayout::AnalogStyle).data);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::OilPressure),static_cast<uint8_t>(dash[1].parameter));
    dash[0].visible=false;
    TEST_ASSERT_TRUE(track[0].visible);
    auto placements=TileEngine::placements(PageId::Dash,config);
    TEST_ASSERT_EQUAL_UINT32(5,placements.count);
    TEST_ASSERT_EQUAL_INT16(464,placements.items[0].geometry.x);
    TEST_ASSERT_EQUAL_INT16(84,placements.items[0].geometry.y);
    TEST_ASSERT_EQUAL_INT16(356,placements.items[4].geometry.y);
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash,1},config));
    editor.setWarning({true,WarningDirection::Below,2.5f,.2f,0});
    TEST_ASSERT_TRUE(editor.applyTo(config));
    VehicleState state;state.set(ParameterId::OilPressure,1.f,100);
    TileWarningEngine warnings;warnings.evaluate(config,state,100);
    TEST_ASSERT_TRUE(warnings.nextModal().has_value());
    config.dash_layout=DashboardLayout::AnalogStyle;
    warnings.evaluate(config,state,120);
    TEST_ASSERT_FALSE(warnings.nextModal().has_value());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_analog_center_is_an_independent_editable_tile_and_can_be_hidden);
    RUN_TEST(test_sixth_layout_has_independent_editable_rows_and_packs_hidden_tiles_down);
    RUN_TEST(test_each_page_can_assign_any_preset_and_keep_its_tile_bank);
    RUN_TEST(test_all_five_presets_fit_both_pages_without_overlapping_navigation);
    RUN_TEST(test_shared_scale_clamps_snaps_and_never_changes_shift_thresholds);
    RUN_TEST(test_invalid_layouts_restore_original_page_choices);
    RUN_TEST(test_editor_and_warnings_use_only_the_selected_bank);
    RUN_TEST(test_hidden_strip_column_tiles_pack_down_without_moving_center);
    RUN_TEST(test_approved_style_placements_preserve_touch_targets_and_frame_spacing);
    return UNITY_END();
}

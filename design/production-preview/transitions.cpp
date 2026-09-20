#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <lvgl.h>
#include "ui/ui.h"
#include "ui/dashboard_layout.h"
#include "board/lvgl_memory.h"

alignas(std::max_align_t) unsigned char pool[LV_MEM_SIZE];
uintptr_t diy_lvgl_memory=reinterpret_cast<uintptr_t>(pool);
static lv_color_t buffers[2][800*480];
static lv_color_t captured[800*480];
static const char* capture_folder=nullptr;
static lv_obj_t* find(lv_obj_t* root,const lv_obj_class_t* type,const char* text=nullptr) {
    if(lv_obj_check_type(root,type) && (!text || !std::strcmp(lv_label_get_text(root),text)))return root;
    for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i) {
        auto* result=find(lv_obj_get_child(root,i),type,text);if(result)return result;
    }
    return nullptr;
}
static lv_obj_t* findContaining(lv_obj_t* root,const char* text) {
    if(lv_obj_check_type(root,&lv_label_class) && std::strstr(lv_label_get_text(root),text))return root;
    for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i) {
        auto* result=findContaining(lv_obj_get_child(root,i),text);if(result)return result;
    }
    return nullptr;
}
static unsigned countType(lv_obj_t* root,const lv_obj_class_t* type) {
    unsigned count=lv_obj_check_type(root,type) ? 1U:0U;
    for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i)count+=countType(lv_obj_get_child(root,i),type);
    return count;
}
static lv_obj_t* button(lv_obj_t* root,const char* text) {
    if(lv_obj_check_type(root,&lv_btn_class)) {
        auto* label=find(root,&lv_label_class,text);if(label)return root;
    }
    for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i) {
        auto* found=button(lv_obj_get_child(root,i),text);if(found)return found;
    }
    return nullptr;
}
static void click(const char* text) {
    auto* target=button(lv_scr_act(),text);assert(target);
    lv_event_send(target,LV_EVENT_CLICKED,nullptr);
}
static void flush(lv_disp_drv_t* driver,const lv_area_t* area,lv_color_t* pixels) {
    if(driver->direct_mode)std::memcpy(captured,pixels,sizeof(captured));
    else for(int y=area->y1;y<=area->y2;y++)for(int x=area->x1;x<=area->x2;x++)captured[y*800+x]=*pixels++;
    lv_disp_flush_ready(driver);
}
static void screenshot(const char* name) {
    if(!capture_folder)return;
    lv_obj_invalidate(lv_scr_act());lv_refr_now(nullptr);
    char path[512];std::snprintf(path,sizeof(path),"%s/editor-%s.ppm",capture_folder,name);
    auto* f=std::fopen(path,"wb");assert(f);std::fprintf(f,"P6\n800 480\n255\n");
    for(auto pixel:captured) {lv_color32_t p;p.full=lv_color_to32(pixel);const unsigned char rgb[]={p.ch.red,p.ch.green,p.ch.blue};std::fwrite(rgb,1,3,f);}
    std::fclose(f);
}
int main(int argc,char** argv) {
    if(argc>2)capture_folder=argv[2];
    lv_init();lv_disp_draw_buf_t buffer;lv_disp_draw_buf_init(&buffer,buffers[0],buffers[1],800*480);
    lv_disp_drv_t driver;lv_disp_drv_init(&driver);driver.hor_res=800;driver.ver_res=480;
    driver.draw_buf=&buffer;driver.direct_mode=argc>3 && !std::strcmp(argv[3],"--partial") ? 0:1;driver.flush_cb=flush;lv_disp_drv_register(&driver);
    AppConfig config=AppConfig::defaults();BoardDisplay board;Ui ui;
    const bool center=argc>1 && !std::strcmp(argv[1],"--center");
    const bool track=argc>1 && !std::strcmp(argv[1],"--track");
    const bool psi=argc>1 && !std::strcmp(argv[1],"--psi");
    const bool tempunits=argc>1 && !std::strcmp(argv[1],"--tempunits");
    const bool racechrono=argc>1 && !std::strcmp(argv[1],"--racechrono");
    const bool units=argc>1 && (!std::strcmp(argv[1],"--units") || psi || tempunits);
    if(units) {
        config.units.pressure=psi ? PressureUnit::Psi:PressureUnit::Kpa;
        config.dash_tiles[0].parameter=ParameterId::OilPressure;
        config.dash_tiles[0].warning={true,WarningDirection::Below,900,200,0};
        if(tempunits) {
            config.units.temperature=TemperatureUnit::Fahrenheit;
            config.dash_tiles[0].parameter=ParameterId::OilTemperature;
            config.dash_tiles[0].warning={true,WarningDirection::Above,120.15f,5.15f,250};
            config.dash_tiles[0].temperature_bar={true,40.1f,75.15f,115.12f,130.3f};
        }
    }
    if(center)config.dash_layout=DashboardLayout::AnalogStyle;
    ui.setDataContext(DataSource::Demo,nullptr);ui.begin(config,board);
    VehicleState state;state.reset(DataSource::Demo);state.set(ParameterId::Rpm,6840,0);
    state.set(ParameterId::Speed,137,0);
    state.set(ParameterId::OilTemperature,108,0);
    RuntimeDiagnostics diagnostics{};UiRuntimeStatus status{};TileWarningEngine warnings;
    ui.update(state,diagnostics,status,config,warnings);ui.updateShiftLight(state,0,config.shift);
    auto* dash=lv_scr_act();
    if(racechrono) {
        click("SETTINGS");click("DATA & CAN");
        assert(button(lv_scr_act(),"RACECHRONO") && "DATA & CAN entry is missing");
        click("RACECHRONO");
        assert(find(lv_scr_act(),&lv_label_class,"RACECHRONO"));
        assert(button(lv_scr_act(),"CONNECTION"));
        assert(button(lv_scr_act(),"CHANNELS"));
        assert(find(lv_scr_act(),&lv_label_class,"ENABLED"));
        screenshot("racechrono-connection");

        click("CHANNELS");
        assert(find(lv_scr_act(),&lv_label_class,"CHANNEL FILTER"));
        assert(find(lv_scr_act(),&lv_label_class,"PAGE 1 / 6"));
        screenshot("racechrono-channels");
        click("NEXT >");
        assert(find(lv_scr_act(),&lv_label_class,"PAGE 2 / 6"));
        click("CONNECTION");

        auto* enabled=find(lv_scr_act(),&lv_checkbox_class);assert(enabled);
        lv_obj_add_state(enabled,LV_STATE_CHECKED);
        lv_event_send(enabled,LV_EVENT_VALUE_CHANGED,nullptr);
        assert(config.racechrono.enabled);
        click("RESTART BLE");
        assert(ui.takeRaceChronoRestart());
        assert(!ui.takeRaceChronoRestart());
        click("< BACK");
        assert(find(lv_scr_act(),&lv_label_class,"DATA & CAN"));
        ConfigCommitRequest request;assert(ui.takeConfigCommit(request));
        assert(request.candidate.racechrono.enabled);
        std::puts("RaceChrono settings navigation, paging and staged save passed");
    } else if(units) {
        lv_event_send(lv_obj_get_child(dash,2),LV_EVENT_LONG_PRESSED,nullptr);
        click("SAVE TILE");ConfigCommitRequest request;assert(ui.takeConfigCommit(request));
        if(tempunits) {
            assert(request.candidate.dash_tiles[0].warning.delay_ms==250 && "Unchanged delay rounded during save");
            assert(std::fabs(request.candidate.dash_tiles[0].temperature_bar.ready_native-75.15f)<.00001f && "Unchanged temperature limit rounded during save");
            assert(std::fabs(request.candidate.dash_tiles[0].warning.threshold_native-120.15f)<.00001f);
        } else {
            assert(std::fabs(request.candidate.dash_tiles[0].warning.hysteresis_native-200)<.00001f && "Converted reset range/rounding changes an untouched warning");
            assert(std::fabs(request.candidate.dash_tiles[0].warning.threshold_native-900)<.00001f);
        }
        std::puts("Unit-aware pressure warning roundtrip passed");
    } else if(argc>1 && !std::strcmp(argv[1],"--editor")) {
        lv_event_send(lv_obj_get_child(dash,2),LV_EVENT_LONG_PRESSED,nullptr);
        assert(button(lv_scr_act(),"DATA") && "Missing full-screen editor tabs");
        screenshot("data");
        assert(lv_obj_has_flag(button(lv_scr_act(),"TEMPERATURE BAR"),LV_OBJ_FLAG_HIDDEN));
        click("WARNING");
        assert(button(lv_scr_act(),"TEST WARNING"));
        screenshot("warning");
        click("TEST WARNING");assert(ui.takeWarningTest());assert(!ui.takeWarningTest());
        screenshot("warning-test");click("CLOSE TEST");
        auto* threshold=find(lv_scr_act(),&lv_spinbox_class);assert(threshold);
        lv_event_send(threshold,LV_EVENT_CLICKED,nullptr);
        assert(find(lv_scr_act(),&lv_label_class,"WARNING THRESHOLD"));
        click("1");click("2");click("0");click(".");click("0");screenshot("numeric");click("APPLY");
        assert(lv_spinbox_get_value(threshold)==1200);
        lv_event_send(threshold,LV_EVENT_CLICKED,nullptr);click("9");click("CANCEL");
        assert(lv_spinbox_get_value(threshold)==1200 && "Keypad CANCEL leaked an edit");
        auto* warning_parent=lv_obj_get_parent(threshold);
        auto* enabled=find(warning_parent,&lv_checkbox_class);assert(enabled);
        lv_obj_add_state(enabled,LV_STATE_CHECKED);
        auto* direction=find(warning_parent,&lv_dropdown_class);assert(direction);
        lv_dropdown_set_selected(direction,1);lv_event_send(direction,LV_EVENT_VALUE_CHANGED,nullptr);
        assert(lv_obj_has_state(enabled,LV_STATE_CHECKED) && "Direction change discarded pending checkbox state");
        click("DATA");click("SELECT PARAMETER");click("TEMPERATURE");screenshot("picker");click("BACK");
        auto* parameter=find(lv_scr_act(),&lv_dropdown_class);assert(parameter);
        auto options=ParameterOptions::build(DataSource::Demo,nullptr,ParameterId::Rpm);
        for(size_t i=0;i<options.count();i++)if(options.parameterAt(i)==ParameterId::OilTemperature)lv_dropdown_set_selected(parameter,i);
        lv_event_send(parameter,LV_EVENT_VALUE_CHANGED,nullptr);
        click("TEMPERATURE BAR");screenshot("temperature");
        click("DATA");screenshot("data");click("WARNING");
        auto* oil_threshold=find(lv_scr_act(),&lv_spinbox_class);
        auto* oil_parent=lv_obj_get_parent(oil_threshold);
        auto* oil_direction=find(oil_parent,&lv_dropdown_class);lv_dropdown_set_selected(oil_direction,0);lv_event_send(oil_direction,LV_EVENT_VALUE_CHANGED,nullptr);
        lv_spinbox_set_value(oil_threshold,1200);
        // RESET is the second spinbox in the warning panel.
        unsigned count=0;
        for(uint32_t i=0;i<lv_obj_get_child_cnt(oil_parent);i++) {
            auto* child=lv_obj_get_child(oil_parent,i);
            if(lv_obj_check_type(child,&lv_spinbox_class) && ++count==2)lv_spinbox_set_value(child,1150);
        }
        lv_obj_add_state(find(oil_parent,&lv_checkbox_class),LV_STATE_CHECKED);
        click("DATA");click("WARNING");screenshot("warning");
        click("CANCEL");assert(lv_scr_act()==dash);
        ConfigCommitRequest request;assert(!ui.takeConfigCommit(request));
        click("SETTINGS");click("RPM & SHIFT LIGHT");
        assert(find(lv_scr_act(),&lv_label_class,"RPM SCALE MAX"));
        ui.update(state,diagnostics,status,config,warnings);
        assert(find(lv_scr_act(),&lv_label_class,"FLASH ENABLED") && "Flash enable switch is missing");
        assert(find(lv_scr_act(),&lv_label_class,"12-LED FILL MAX") && "12-LED fill maximum slider is missing");
        assert(countType(lv_scr_act(),&lv_slider_class)==5 && "RPM settings must expose five sliders");
        assert(!find(lv_scr_act(),&lv_label_class,"ADVANCED: 12-LED FILL MAX") && "Duplicate lower-right RPM field remains");
        assert(!findContaining(lv_scr_act(),"UI updates:") && "Runtime diagnostics overwrite RPM status");
        assert(find(lv_scr_act(),&lv_label_class,"Colors follow YELLOW / RED thresholds; 12 LEDs retain 4/4/4 zones"));
        screenshot("rpm");
        click("8000 RPM");click("9");click("0");click("0");click("0");click("APPLY");
        assert(config.shift.max_rpm==9000);
        click("10000 RPM");click("8");click("0");click("0");click("0");click("APPLY");
        assert(config.rpm_scale_max==8000 && config.shift.flash_rpm==7500);
        click("TRACK");assert(ui.takeConfigCommit(request));assert(request.candidate.rpm_scale_max==8000 && request.candidate.shift.max_rpm==9000);
        std::puts("Tabbed editor cancel and unified RPM navigation passed");
    } else if(argc==1 || track) {
        if(track) { click("TRACK");dash=lv_scr_act(); }
        click("SETTINGS");click("LAYOUTS");
        auto* dropdown=find(lv_scr_act(),&lv_dropdown_class);assert(dropdown);
        if(track) {
            auto* parent=lv_obj_get_parent(dropdown);
            for(uint32_t i=0;i<lv_obj_get_child_cnt(parent);++i) {
                auto* child=lv_obj_get_child(parent,i);
                if(child!=dropdown && lv_obj_check_type(child,&lv_dropdown_class)) { dropdown=child;break; }
            }
        }
        lv_dropdown_set_selected(dropdown,4);lv_event_send(dropdown,LV_EVENT_VALUE_CHANGED,nullptr);
        click(track ? "TRACK":"DASH");assert(lv_scr_act()==dash);
        // Assert before any scheduled Ui::update: the first visible screen must be Strip.
        assert(lv_obj_get_width(lv_obj_get_child(dash,1))==784 && "First visible frame still uses the old layout");
        assert(find(dash,&lv_label_class,"6840") && "RPM missing from first frame");
        std::puts("Layout return first-frame test passed");
    } else if(argc>1 && !std::strcmp(argv[1],"--retry")) {
        auto* tile=lv_obj_get_child(dash,2);lv_event_send(tile,LV_EVENT_LONG_PRESSED,nullptr);
        auto* editor=lv_scr_act();auto* visible=find(editor,&lv_checkbox_class);lv_obj_clear_state(visible,LV_STATE_CHECKED);
        click("SAVE TILE");ConfigCommitRequest request;assert(ui.takeConfigCommit(request));
        ui.completeConfigCommit(request.revision,false);
        assert(lv_scr_act()==editor && config.dash_tiles[0].visible);
        assert(!lv_obj_has_state(button(editor,"SAVE TILE"),LV_STATE_DISABLED));
        click("SAVE TILE");assert(ui.takeConfigCommit(request));config=request.candidate;ui.completeConfigCommit(request.revision,true);
        assert(lv_scr_act()==dash && !config.dash_tiles[0].visible);
        std::puts("Failed tile save stays editable and retries transactionally");
    } else if(center) {
        auto* tile=lv_obj_get_child(dash,8); // configurable Analog slot 6
        lv_event_send(tile,LV_EVENT_LONG_PRESSED,nullptr);
        assert(find(lv_scr_act(),&lv_label_class,"TILE SETTINGS"));
        auto* dropdown=find(lv_scr_act(),&lv_dropdown_class);assert(dropdown);
        lv_dropdown_set_selected(dropdown,9); // SPEED in the complete Demo parameter list
        lv_event_send(dropdown,LV_EVENT_VALUE_CHANGED,nullptr);
        click("SAVE TILE");ConfigCommitRequest request;assert(ui.takeConfigCommit(request));
        config=request.candidate;ui.completeConfigCommit(request.revision,true);
        assert(activeTiles(config,PageId::Dash)[6].parameter==ParameterId::Speed);
        assert(find(tile,&lv_label_class,"137") && "Analog center shows old RPM after saving SPEED");
        std::puts("Analog center long-press, parameter choice and first-frame save test passed");
    } else {
        // Open the real first tile through its registered long-press event.
        auto* tile=lv_obj_get_child(dash,2);
        lv_event_send(tile,LV_EVENT_LONG_PRESSED,nullptr);
        assert(find(lv_scr_act(),&lv_label_class,"TILE SETTINGS"));
        auto* visible=find(lv_scr_act(),&lv_checkbox_class);assert(visible);
        lv_obj_clear_state(visible,LV_STATE_CHECKED);
        click("SAVE TILE");ConfigCommitRequest request;assert(ui.takeConfigCommit(request));
        config=request.candidate;ui.completeConfigCommit(request.revision,true);
        assert(lv_scr_act()==dash);
        assert(lv_obj_has_flag(tile,LV_OBJ_FLAG_HIDDEN) && "Saved tile visibility applied after screen return");
        std::puts("Tile-save return first-frame test passed");
    }
}

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <lvgl.h>
#include "ui/ui.h"
#include "board/lvgl_memory.h"

alignas(std::max_align_t) unsigned char pool[LV_MEM_SIZE];
uintptr_t diy_lvgl_memory=reinterpret_cast<uintptr_t>(pool);
static lv_color_t buffers[2][800*480];
static lv_obj_t* find(lv_obj_t* root,const lv_obj_class_t* type,const char* text=nullptr) {
    if(lv_obj_check_type(root,type) && (!text || !std::strcmp(lv_label_get_text(root),text)))return root;
    for(uint32_t i=0;i<lv_obj_get_child_cnt(root);++i) {
        auto* result=find(lv_obj_get_child(root,i),type,text);if(result)return result;
    }
    return nullptr;
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
static void flush(lv_disp_drv_t* driver,const lv_area_t*,lv_color_t*) { lv_disp_flush_ready(driver); }
int main(int argc,char**) {
    lv_init();lv_disp_draw_buf_t buffer;lv_disp_draw_buf_init(&buffer,buffers[0],buffers[1],800*480);
    lv_disp_drv_t driver;lv_disp_drv_init(&driver);driver.hor_res=800;driver.ver_res=480;
    driver.draw_buf=&buffer;driver.direct_mode=1;driver.flush_cb=flush;lv_disp_drv_register(&driver);
    AppConfig config=AppConfig::defaults();BoardDisplay board;Ui ui;ui.begin(config,board);
    VehicleState state;state.reset(DataSource::Demo);state.set(ParameterId::Rpm,6840,0);
    RuntimeDiagnostics diagnostics{};UiRuntimeStatus status{};TileWarningEngine warnings;
    ui.update(state,diagnostics,status,config,warnings);ui.updateShiftLight(state,0,config.shift);
    auto* dash=lv_scr_act();
    if(argc==1) {
        click("SETTINGS");click("LAYOUTS");
        auto* dropdown=find(lv_scr_act(),&lv_dropdown_class);assert(dropdown);
        lv_dropdown_set_selected(dropdown,4);lv_event_send(dropdown,LV_EVENT_VALUE_CHANGED,nullptr);
        click("DASH");assert(lv_scr_act()==dash);
        // Assert before any scheduled Ui::update: the first visible screen must be Strip.
        assert(lv_obj_get_width(lv_obj_get_child(dash,1))==784 && "First visible frame still uses the old layout");
        assert(find(dash,&lv_label_class,"6840") && "RPM missing from first frame");
        std::puts("Layout return first-frame test passed");
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

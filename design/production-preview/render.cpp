#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <lvgl.h>
#include "settings/app_config.h"
#include "ui/dashboard_layout.h"
#include "ui/tile_engine.h"
#include "ui/tile_view.h"
#include "ui/rpm_scale_view.h"
#include "ui/shift_light_view.h"
#include "ui/navigation_view.h"
#include "ui/ui_theme.h"

namespace {
unsigned char framebuffer[480][800][3];
lv_color_t draw_buffer[800*40];
void flush(lv_disp_drv_t* driver,const lv_area_t* area,lv_color_t* pixels) {
    for(int y=area->y1;y<=area->y2;++y)for(int x=area->x1;x<=area->x2;++x) {
        lv_color32_t pixel; pixel.full=lv_color_to32(*pixels++);
        if(x>=0 && x<800 && y>=0 && y<480) {
            framebuffer[y][x][0]=pixel.ch.red; framebuffer[y][x][1]=pixel.ch.green;
            framebuffer[y][x][2]=pixel.ch.blue;
        }
    }
    lv_disp_flush_ready(driver);
}
void emptyEvent(lv_event_t*) {}
bool pixelMatches(int x,int y,lv_color_t expected) {
    lv_color32_t c; c.full=lv_color_to32(expected);
    return framebuffer[y][x][0]==c.ch.red && framebuffer[y][x][1]==c.ch.green && framebuffer[y][x][2]==c.ch.blue;
}
void save(const char* directory,const char* name) {
    char path[512]; std::snprintf(path,sizeof(path),"%s/%s.ppm",directory,name);
    FILE* out=std::fopen(path,"wb"); assert(out);
    std::fprintf(out,"P6\n800 480\n255\n");
    assert(std::fwrite(framebuffer,1,sizeof(framebuffer),out)==sizeof(framebuffer));
    std::fclose(out);
}
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;
    lv_init(); lv_disp_draw_buf_t buffer; lv_disp_draw_buf_init(&buffer,draw_buffer,nullptr,800*40);
    lv_disp_drv_t driver; lv_disp_drv_init(&driver); driver.hor_res=800; driver.ver_res=480;
    driver.draw_buf=&buffer; driver.flush_cb=flush; lv_disp_drv_register(&driver);
    auto* screen=lv_scr_act(); lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen,UiTheme::background(),0); lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    RpmScaleView rpm; rpm.create(screen); ShiftLightView shift; shift.create(screen);
    std::array<TileView,14> tiles;
    for(unsigned i=0;i<tiles.size();++i)tiles[i].create(screen,{PageId::Dash,static_cast<uint8_t>(i)},emptyEvent);
    const auto navigation_index=lv_obj_get_child_cnt(screen);
    VehicleState state; state.reset(DataSource::Demo);
    state.set(ParameterId::Rpm,6840,0); state.set(ParameterId::Speed,137,0);
    state.set(ParameterId::Gear,3,0); state.set(ParameterId::OilPressure,4.8,0);
    state.set(ParameterId::OilTemperature,108,0); state.set(ParameterId::Clt,91,0);
    state.set(ParameterId::Iat,32,0); state.set(ParameterId::Map,1.18,0);
    state.set(ParameterId::Lambda,.86,0); state.set(ParameterId::FuelPressure,4.1,0);
    state.set(ParameterId::BatteryVoltage,14.2,0); state.set(ParameterId::Tps,78,0);
    const char* names[]={"firmware-classic-dash","firmware-classic-track","firmware-analog-style",
        "firmware-side-gear","firmware-strip-style","firmware-analog-flags","firmware-warning-tile",
        "firmware-scale-6000","firmware-flash-on","firmware-flash-off","firmware-compact-flags",
        "firmware-rpm-10000","firmware-analog-unavailable","firmware-strip-unavailable"};
    for(int scenario=0;scenario<14;++scenario) {
        AppConfig config=AppConfig::defaults();
        config.dash_layout=scenario<5 ? static_cast<DashboardLayout>(scenario):
            scenario==5 || scenario==12 ? DashboardLayout::AnalogStyle:DashboardLayout::StripStyle;
        auto bank=activeTiles(config,PageId::Dash);
        if(scenario==5 || scenario==10) {
            for(int i=0;i<3;++i)bank[i].parameter=ParameterId::CheckEngine;
            bank[0].flag_active_color=FlagActiveColor::Red;
            bank[1].parameter=ParameterId::LaunchControlActive;
            bank[2].parameter=ParameterId::AntiLagActive;
            state.set(ParameterId::CheckEngine,1,0); state.set(ParameterId::LaunchControlActive,0,0);
            state.invalidate(ParameterId::AntiLagActive);
        }
        if(scenario==7)config.rpm_scale_max=6000;
        state.set(ParameterId::Rpm,scenario==11 ? 10000:scenario>=8 && scenario<=9 ? 8500:6840,0);
        const uint32_t now=scenario==9 ? 125:0;
        for(auto& tile:tiles)tile.hide();
        rpm.apply(config.dash_layout,config.rpm_scale_max); shift.apply(config.dash_layout);
        while(lv_obj_get_child_cnt(screen)>navigation_index)lv_obj_del(lv_obj_get_child(screen,navigation_index));
        createNavigationView(screen,scenario==1 ? 1:0,nullptr);
        const auto placements=TileEngine::placements(PageId::Dash,config);
        for(unsigned i=0;i<placements.count;++i) {
            const auto& p=placements.items[i]; auto& view=tiles[p.address.slot];
            view.apply(bank[p.address.slot],p.geometry);
        }
        lv_obj_update_layout(screen);
        for(unsigned i=0;i<placements.count;++i) {
            const auto slot=placements.items[i].address.slot;
            const bool supported=!(scenario==12 && slot==1) && !(scenario==13 && slot==0);
            tiles[slot].update(bank[slot],config.units,state,supported,scenario==6 && slot==1,now);
        }
        rpm.update(state.get(ParameterId::Rpm),now,config.shift);
        shift.update(static_cast<uint16_t>(state.get(ParameterId::Rpm).value),true,now,config.shift);
        lv_obj_update_layout(screen);
        if(scenario==5 || scenario==10) {
            // Guard against unavailable captions overlapping flag pills/values.
            auto* root=lv_obj_get_child(screen,4); // third tile, first two children are RPM/shift
            auto* value=lv_obj_get_child(root,2);
            auto* unit=lv_obj_get_child(root,3);
            lv_area_t v,u; lv_obj_get_coords(value,&v); lv_obj_get_coords(unit,&u);
            assert(v.x2<u.x1 || u.x2<v.x1 || v.y2<u.y1 || u.y2<v.y1);
        }
        if(scenario==11) {
            auto* root=lv_obj_get_child(screen,5); // RPM hero slot 3
            auto* value=lv_obj_get_child(root,2);
            lv_point_t measured;
            lv_txt_get_size(&measured,lv_label_get_text(value),lv_obj_get_style_text_font(value,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
            assert(measured.x<=lv_obj_get_width(value));
        }
        if(scenario==12 || scenario==13) {
            auto* root=lv_obj_get_child(screen,scenario==12 ? 3:2);
            auto* unit=lv_obj_get_child(root,3);
            lv_area_t r,u; lv_obj_get_coords(root,&r); lv_obj_get_coords(unit,&u);
            assert(u.x1>=r.x1 && u.x2<=r.x2 && u.y1>=r.y1 && u.y2<=r.y2);
            assert(std::strcmp(lv_label_get_text(unit),"UNAVAILABLE")==0);
        }
        std::memset(framebuffer,0,sizeof(framebuffer)); lv_obj_invalidate(screen); lv_refr_now(nullptr);
        if(scenario==8)assert(pixelMatches(410,50,UiTheme::red()));
        if(scenario==9)assert(pixelMatches(410,50,lv_color_hex(0x151D22)));
        save(argv[1],names[scenario]);
    }
    std::puts("Rendered 14 real production-view framebuffers; flash, flags, font fit and unavailable assertions passed.");
    return 0;
}

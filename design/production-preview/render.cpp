#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
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
#include "board/lvgl_memory.h"

alignas(std::max_align_t) static unsigned char preview_pool[LV_MEM_SIZE];
uintptr_t diy_lvgl_memory=reinterpret_cast<uintptr_t>(preview_pool);

namespace {
unsigned char framebuffer[480][800][3];
lv_color_t draw_buffer[800*40];
lv_color_t direct_buffers[2][800*480];
unsigned char incremental_frame[480][800][3];
unsigned flushed_pixels=0;
void flush(lv_disp_drv_t* driver,const lv_area_t* area,lv_color_t* pixels) {
    if(driver->direct_mode && !lv_disp_flush_is_last(driver)) {
        lv_disp_flush_ready(driver);return;
    }
    for(int y=area->y1;y<=area->y2;++y)for(int x=area->x1;x<=area->x2;++x) {
        lv_color32_t pixel;
        pixel.full=lv_color_to32(driver->direct_mode ? pixels[y*800+x]:*pixels++);
        if(x>=0 && x<800 && y>=0 && y<480) {
            framebuffer[y][x][0]=pixel.ch.red; framebuffer[y][x][1]=pixel.ch.green;
            framebuffer[y][x][2]=pixel.ch.blue;
        }
    }
    lv_disp_flush_ready(driver);
}
void emptyEvent(lv_event_t*) {}
void monitor(lv_disp_drv_t*,uint32_t,uint32_t pixels) { flushed_pixels=pixels; }
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
void compareIncremental(lv_obj_t* screen,const char* scene,unsigned step) {
    lv_obj_update_layout(screen);lv_refr_now(nullptr);
    std::memcpy(incremental_frame,framebuffer,sizeof(framebuffer));
    lv_obj_invalidate(screen);lv_refr_now(nullptr);
    if(std::memcmp(incremental_frame,framebuffer,sizeof(framebuffer))) {
        for(int y=0;y<480;y++)for(int x=0;x<800;x++) {
            if(std::memcmp(incremental_frame[y][x],framebuffer[y][x],3)) {
                std::fprintf(stderr,"Partial redraw differs: %s step %u pixel %d,%d\n",scene,step,x,y);
                assert(false);
            }
        }
    }
}
}

int main(int argc,char** argv) {
    if(argc!=2 && argc!=3)return 2;
    const bool direct=argc==3 && std::strcmp(argv[2],"--direct")==0;
    lv_init(); lv_disp_draw_buf_t buffer;
    lv_disp_draw_buf_init(&buffer,direct ? direct_buffers[0]:draw_buffer,
        direct ? direct_buffers[1]:nullptr,direct ? 800*480:800*40);
    lv_disp_drv_t driver; lv_disp_drv_init(&driver); driver.hor_res=800; driver.ver_res=480;
    driver.draw_buf=&buffer; driver.flush_cb=flush; driver.direct_mode=direct;driver.monitor_cb=monitor;
    lv_disp_drv_register(&driver);
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
        "firmware-rpm-10000","firmware-analog-unavailable","firmware-strip-unavailable",
        "firmware-modern-motorsport","firmware-modern-7500","firmware-modern-idle",
        "firmware-modern-redline","firmware-modern-flash-on","firmware-modern-flash-off",
        "firmware-modern-unavailable","firmware-modern-flags","firmware-modern-hidden-warning",
        "firmware-analog-7500","firmware-strip-7500"};
    constexpr int scene_count=sizeof(names)/sizeof(names[0]);
    for(int scenario=0;scenario<scene_count;++scenario) {
        AppConfig config=AppConfig::defaults();
        config.dash_layout=scenario<5 ? static_cast<DashboardLayout>(scenario):
            scenario>=14 && scenario<=22 ? DashboardLayout::ModernMotorsport:
            scenario==5 || scenario==12 || scenario==23 ? DashboardLayout::AnalogStyle:DashboardLayout::StripStyle;
        auto bank=activeTiles(config,PageId::Dash);
        if(scenario==5 || scenario==10 || scenario==21) {
            for(int i=0;i<3;++i)bank[i].parameter=ParameterId::CheckEngine;
            bank[0].flag_active_color=FlagActiveColor::Red;
            bank[1].parameter=ParameterId::LaunchControlActive;
            bank[2].parameter=ParameterId::AntiLagActive;
            state.set(ParameterId::CheckEngine,1,0); state.set(ParameterId::LaunchControlActive,0,0);
            state.invalidate(ParameterId::AntiLagActive);
        }
        if(scenario==7)config.rpm_scale_max=6000;
        if(scenario==15 || scenario==23 || scenario==24)config.rpm_scale_max=7500;
        if(scenario==17)config.shift.flash_enabled=false;
        if(scenario==22)bank[0].visible=false;
        state.set(ParameterId::Rpm,scenario==16 ? 1200:scenario==11 ? 10000:
            (scenario>=8 && scenario<=9) || (scenario>=17 && scenario<=19) ? 8500:6840,0);
        const uint32_t now=scenario==9 || scenario==19 ? 125:0;
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
            const bool supported=!(scenario==12 && slot==1) && !(scenario==13 && slot==0) && !(scenario==20 && slot==1);
            tiles[slot].update(bank[slot],config.units,state,supported,(scenario==6 || scenario==22) && slot==1,now);
        }
        rpm.update(state.get(ParameterId::Rpm),now,config.shift);
        shift.update(static_cast<uint16_t>(state.get(ParameterId::Rpm).value),true,now,config.shift);
        lv_obj_update_layout(screen);
        if(scenario==5 || scenario==10 || scenario==21) {
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
        if(scenario==12 || scenario==13 || scenario==20) {
            auto* root=lv_obj_get_child(screen,scenario==13 ? 2:3);
            auto* unit=lv_obj_get_child(root,3);
            lv_area_t r,u; lv_obj_get_coords(root,&r); lv_obj_get_coords(unit,&u);
            assert(u.x1>=r.x1 && u.x2<=r.x2 && u.y1>=r.y1 && u.y2<=r.y2);
            assert(std::strcmp(lv_label_get_text(unit),"UNAVAILABLE")==0);
        }
        std::memset(framebuffer,0,sizeof(framebuffer)); lv_obj_invalidate(screen); lv_refr_now(nullptr);
        if(scenario==8)assert(pixelMatches(410,50,UiTheme::red()));
        if(scenario==9)assert(pixelMatches(410,50,lv_color_hex(0x151D22)));
        save(argv[1],names[scenario]);
        if(scenario==2) {
            state.set(ParameterId::Rpm,8500,0);
            rpm.update(state.get(ParameterId::Rpm),0,config.shift);
            lv_refr_now(nullptr);
            for(int segment=0;segment<54;++segment) {
                if(segment>=44 && segment<=47)continue; // permanent white index near 364 degrees
                const float radians=(137+5*segment)*3.14159265358979323846f/180;
                const int x=std::lround(228+197*std::cos(radians));
                const int y=std::lround(220+197*std::sin(radians));
                assert(pixelMatches(x,y,UiTheme::red()) && "Analog flash does not light the whole segmented ring");
            }
            save(argv[1],"analog-ring-flash-on");
            rpm.update(state.get(ParameterId::Rpm),125,config.shift);lv_refr_now(nullptr);
            for(int segment=0;segment<54;++segment) {
                if(segment>=44 && segment<=47)continue;
                const float radians=(137+5*segment)*3.14159265358979323846f/180;
                assert(pixelMatches(std::lround(228+197*std::cos(radians)),std::lround(220+197*std::sin(radians)),lv_color_hex(0x151D22)));
            }
            save(argv[1],"analog-ring-flash-off");
            state.set(ParameterId::Rpm,6840,0);rpm.update(state.get(ParameterId::Rpm),250,config.shift);
        }
        if(scenario==18) {
            flushed_pixels=0;
            rpm.update(state.get(ParameterId::Rpm),125,config.shift);
            lv_refr_now(nullptr);
            assert(flushed_pixels<=120000 && "Modern flash repaints too much of the screen");
            for(int angle=185;angle<355;angle+=5) {
                const float radians=angle*3.14159265358979323846f/180;
                const int x=std::lround(228+203*std::cos(radians));
                const int y=std::lround(280+203*std::sin(radians));
                // Avoid the white progress cap at 8500 RPM.
                if(angle<330 || angle>336)assert(pixelMatches(x,y,lv_color_hex(0x151D22)));
            }
        }
        if(scenario==2 || scenario==3 || scenario==4 || scenario==14) {
            if(scenario==2 || scenario==14)
                for(int row=0;row<6;row++)assert(pixelMatches(472,32+68*row,lv_color_hex(0x707780)) && "Tile rails are not neutral grey");
            const float values[]={200,210,220,6860,5500,5499,7000,10000,0,1200,8500,8500,6840};
            for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++) {
                state.set(ParameterId::Rpm,values[i],(i+1)*125);
                rpm.update(state.get(ParameterId::Rpm),(i+1)*125,config.shift);
                compareIncremental(screen,names[scenario],i);
            }
            state.invalidate(ParameterId::Rpm);
            rpm.update(state.get(ParameterId::Rpm),1500,config.shift);
            compareIncremental(screen,names[scenario],10);
        }
    }
    std::printf("Rendered %d layout scenes plus 2 Analog flash scenes; incremental redraws, flash, flags, font fit, grey rails and unavailable assertions passed.\n",scene_count);
    return 0;
}

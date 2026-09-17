#include "navigation_view.h"
#include <cstdint>
#include "navigation_icon.h"
#include "tile_layout.h"
#include "ui_theme.h"

namespace {
void separators(lv_event_t* event) {
    auto* ctx=lv_event_get_draw_ctx(event); lv_area_t a;
    lv_obj_get_coords(lv_event_get_target(event),&a);
    const int i=static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d); d.color=UiTheme::border(); d.width=1;
    lv_point_t p{static_cast<lv_coord_t>(a.x1+(i==0 ? 8:0)),static_cast<lv_coord_t>(a.y1+4)};
    lv_point_t q{static_cast<lv_coord_t>(a.x2-(i==2 ? 8:0)),p.y}; lv_draw_line(ctx,&d,&p,&q);
    if(i<2) {
        p={a.x2,static_cast<lv_coord_t>(a.y1+14)};
        q={a.x2,static_cast<lv_coord_t>(a.y2-6)}; lv_draw_line(ctx,&d,&p,&q);
    }
}
}
void createNavigationView(lv_obj_t* parent,int active,lv_event_cb_t callback) {
    constexpr const char* labels[]={"DASH","TRACK","SETTINGS"};
    constexpr int widths[]={267,266,267}; int x=0;
    for(int i=0;i<3;++i) {
        auto* button=lv_btn_create(parent); lv_obj_set_pos(button,x,TileLayout::kNavigationY);
        lv_obj_set_size(button,widths[i],TileLayout::kNavigationHeight); x+=widths[i];
        lv_obj_set_style_pad_all(button,0,0); lv_obj_set_style_radius(button,0,0);
        lv_obj_set_style_shadow_width(button,0,0); lv_obj_set_style_border_width(button,0,0);
        lv_obj_set_style_bg_color(button,UiTheme::background(),0);
        lv_obj_set_style_text_color(button,i==active ? UiTheme::blue():UiTheme::muted(),0);
        void* index=reinterpret_cast<void*>(static_cast<intptr_t>(i));
        lv_obj_add_event_cb(button,separators,LV_EVENT_DRAW_MAIN,index);
        lv_obj_add_event_cb(button,drawNavigationIcon,LV_EVENT_DRAW_MAIN,index);
        if(callback)lv_obj_add_event_cb(button,callback,LV_EVENT_CLICKED,index);
        auto* label=lv_label_create(button); lv_label_set_text(label,labels[i]);
        lv_obj_set_style_text_font(label,&lv_font_montserrat_20,0);
        lv_obj_align(label,LV_ALIGN_CENTER,i==2 ? 38:6,0);
        if(i==active) {
            auto* underline=lv_obj_create(button); lv_obj_remove_style_all(underline);
            lv_obj_set_size(underline,168,3); lv_obj_set_style_bg_color(underline,UiTheme::blue(),0);
            lv_obj_set_style_bg_opa(underline,LV_OPA_COVER,0);
            lv_obj_clear_flag(underline,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(underline,LV_ALIGN_BOTTOM_MID,0,1);
        }
    }
}

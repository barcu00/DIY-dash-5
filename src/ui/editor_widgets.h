#pragma once
#include <lvgl.h>
#include "ui/ui_theme.h"
namespace EditorWidgets {
inline lv_obj_t* label(lv_obj_t* parent,const char* text,int x,int y,
    const lv_font_t* font=&lv_font_montserrat_14,lv_color_t color=UiTheme::text()) {
    auto* o=lv_label_create(parent);lv_label_set_text(o,text);lv_obj_set_pos(o,x,y);
    lv_obj_set_style_text_font(o,font,0);lv_obj_set_style_text_color(o,color,0);return o;
}
inline lv_obj_t* button(lv_obj_t* parent,const char* text,int x,int y,int w,int h,
    lv_event_cb_t cb,intptr_t action=0,bool accent=false) {
    auto* o=lv_btn_create(parent);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,accent ? UiTheme::blue():UiTheme::panel(),0);
    lv_obj_set_style_border_color(o,UiTheme::border(),0);lv_obj_set_style_border_width(o,1,0);
    lv_obj_set_style_shadow_width(o,0,0);lv_obj_set_style_radius(o,5,0);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,reinterpret_cast<void*>(action));
    auto* t=label(o,text,0,0);lv_obj_set_style_text_color(t,accent ? UiTheme::background():UiTheme::text(),0);lv_obj_center(t);return o;
}
inline lv_obj_t* panel(lv_obj_t* parent,int x,int y,int w,int h) {
    auto* o=lv_obj_create(parent);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_pad_all(o,0,0);lv_obj_set_style_bg_color(o,UiTheme::background(),0);
    lv_obj_set_style_border_width(o,0,0);lv_obj_set_style_radius(o,0,0);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
inline void dropdownPopup(lv_event_t* e) {
    auto* list=lv_dropdown_get_list(lv_event_get_target(e));if(!list)return;
    lv_obj_set_style_bg_color(list,UiTheme::panel(),0);lv_obj_set_style_text_color(list,UiTheme::text(),0);
    lv_obj_set_style_border_color(list,UiTheme::border(),0);lv_obj_set_style_border_width(list,1,0);
    lv_obj_set_style_bg_color(list,UiTheme::blue(),LV_PART_SELECTED);
    lv_obj_set_style_text_color(list,UiTheme::background(),LV_PART_SELECTED);
}
inline void darkDropdown(lv_obj_t* o) {
    lv_obj_set_style_bg_color(o,UiTheme::panel(),0);lv_obj_set_style_text_color(o,UiTheme::text(),0);
    lv_obj_set_style_border_color(o,UiTheme::border(),0);lv_obj_set_style_border_width(o,1,0);
    lv_obj_set_style_radius(o,5,0);lv_obj_add_event_cb(o,dropdownPopup,LV_EVENT_CLICKED,nullptr);
}
inline void darkCheckbox(lv_obj_t* o) {
    lv_obj_set_style_text_color(o,UiTheme::text(),0);lv_obj_set_style_text_font(o,&lv_font_montserrat_14,0);
    lv_obj_set_style_bg_color(o,UiTheme::panel(),LV_PART_INDICATOR);
    lv_obj_set_style_border_color(o,UiTheme::border(),LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(o,UiTheme::blue(),LV_PART_INDICATOR|LV_STATE_CHECKED);
}
inline lv_obj_t* field(lv_obj_t* parent,int x,int y,int w,int32_t min,int32_t max,
    int32_t value,uint8_t digits=6,uint8_t separator=5) {
    auto* o=lv_spinbox_create(parent);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,52);
    lv_spinbox_set_range(o,min,max);lv_spinbox_set_digit_format(o,digits,separator);
    lv_spinbox_set_value(o,value);lv_obj_set_style_text_font(o,&lv_font_montserrat_24,0);
    lv_obj_set_style_text_color(o,UiTheme::text(),0);lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_bg_color(o,UiTheme::panel(),0);lv_obj_set_style_border_color(o,UiTheme::border(),0);
    lv_obj_set_style_border_width(o,1,0);return o;
}
}

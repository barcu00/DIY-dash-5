#include "ui.h"
#include <cstdio>
#include "ui/editor_widgets.h"
#include "ui/settings_actions.h"
using namespace EditorWidgets;
void Ui::createShiftSettings(lv_obj_t* parent) {
    // Preview uses the same color thresholds as the tachometer views.
    for(int i=0;i<36;i++) {
        auto* block=panel(parent,14+i*21,0,17,22);
        rpm_preview_blocks_[i]=block;
        const unsigned rpm=config_->rpm_scale_max*(2*i+1)/72;
        lv_obj_set_style_bg_color(block,rpm>=config_->shift.red_rpm ? UiTheme::red():rpm>=config_->shift.start_rpm ? UiTheme::yellow():UiTheme::green(),0);
    }
    label(parent,"Shared by DASH and TRACK | Preview only, not an ECU limiter",14,34,&lv_font_montserrat_12,UiTheme::muted());
    settings_status_=label(parent,"",14,52,&lv_font_montserrat_12,UiTheme::yellow());
    const char* labels[]={"RPM SCALE MAX","YELLOW FROM","RED FROM","FLASH FROM","12-LED FILL MAX"};
    lv_obj_t** sliders[]={&rpm_scale_slider_,&shift_start_,&shift_red_,&shift_flash_,&shift_max_};
    lv_obj_t** values[]={&rpm_scale_value_,&shift_start_value_,&shift_red_value_,&shift_flash_value_,&shift_max_value_};
    const intptr_t actions[]={RpmScaleChanged,ShiftStartChanged,ShiftRedChanged,ShiftFlashChanged,ShiftMaxChanged};
    const uint16_t initial[]={config_->rpm_scale_max,config_->shift.start_rpm,config_->shift.red_rpm,config_->shift.flash_rpm,config_->shift.max_rpm};
    for(int i=0;i<5;i++) {
        const int y=76+i*43;label(parent,labels[i],14,y+8,&lv_font_montserrat_12,UiTheme::muted());
        *sliders[i]=lv_slider_create(parent);lv_obj_set_pos(*sliders[i],184,y+16);lv_obj_set_size(*sliders[i],370,14);
        lv_obj_set_style_bg_color(*sliders[i],UiTheme::blue(),LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(*sliders[i],UiTheme::blue(),LV_PART_KNOB);
        lv_slider_set_range(*sliders[i],i==0 ? 100:0,10000);lv_slider_set_value(*sliders[i],initial[i],LV_ANIM_OFF);
        lv_obj_add_event_cb(*sliders[i],settingsEvent,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(actions[i]));
        bindNumeric(*sliders[i],labels[i],i==0 ? 100:0,10000,0,"RPM",true);
        auto* value=button(parent,"",590,y-1,160,38,numericOpenEvent,i);
        *values[i]=lv_obj_get_child(value,0);
    }
    label(parent,"FLASH ENABLED",14,322,&lv_font_montserrat_12,UiTheme::muted());
    shift_flash_enabled_=lv_switch_create(parent);lv_obj_set_pos(shift_flash_enabled_,146,314);
    if(config_->shift.flash_enabled)lv_obj_add_state(shift_flash_enabled_,LV_STATE_CHECKED);
    lv_obj_add_event_cb(shift_flash_enabled_,settingsEvent,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(ShiftFlashEnabledChanged));
    label(parent,"4 Hz full-strip blink above FLASH FROM",220,320,&lv_font_montserrat_12,UiTheme::muted());
    refreshShiftControls();
}
void Ui::refreshShiftControls() {
    if(!config_)return;
    for(int i=0;i<36;i++)if(rpm_preview_blocks_[i]) {
        const unsigned rpm=config_->rpm_scale_max*(2*i+1)/72;
        lv_obj_set_style_bg_color(rpm_preview_blocks_[i],rpm>=config_->shift.red_rpm ? UiTheme::red():rpm>=config_->shift.start_rpm ? UiTheme::yellow():UiTheme::green(),0);
    }
    lv_obj_t* sliders[]={rpm_scale_slider_,shift_start_,shift_red_,shift_flash_,shift_max_};
    lv_obj_t* labels[]={rpm_scale_value_,shift_start_value_,shift_red_value_,shift_flash_value_,shift_max_value_};
    const uint16_t values[]={config_->rpm_scale_max,config_->shift.start_rpm,config_->shift.red_rpm,config_->shift.flash_rpm,config_->shift.max_rpm};
    for(int i=0;i<5;i++) {
        if(sliders[i])lv_slider_set_value(sliders[i],values[i],LV_ANIM_OFF);
        if(labels[i]) {char text[24];std::snprintf(text,sizeof(text),"%u RPM",values[i]);lv_label_set_text(labels[i],text);}
    }
    if(settings_status_)lv_label_set_text(settings_status_,config_->shift.flash_rpm>config_->rpm_scale_max ? "FLASH threshold is beyond the visible RPM scale":"Colors follow YELLOW / RED thresholds; 12 LEDs retain 4/4/4 zones");
}

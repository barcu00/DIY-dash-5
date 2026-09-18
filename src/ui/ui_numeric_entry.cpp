#include "ui.h"
#include <cmath>
#include <cstdio>
#include "ui/editor_widgets.h"
#include "ui/editor_value_model.h"
using namespace EditorWidgets;
void Ui::bindNumeric(lv_obj_t* widget,const char* title,float minimum,float maximum,
    uint8_t decimals,const char* unit,bool slider) {
    if(numeric_binding_count_>=numeric_bindings_.size())return;
    lv_obj_remove_event_cb(widget,numericOpenEvent);
    const auto index=numeric_binding_count_++;
    numeric_bindings_[index]={widget,title,minimum,maximum,decimals,unit,slider};
    if(!slider) {
        lv_obj_add_event_cb(widget,numericOpenEvent,LV_EVENT_CLICKED,reinterpret_cast<void*>(index));
        // Hide spinbox's zero-padded text; its child presents a normal decimal value.
        lv_obj_set_style_text_opa(widget,LV_OPA_TRANSP,0);
        lv_obj_set_style_opa(widget,LV_OPA_TRANSP,LV_PART_CURSOR);
        if(lv_obj_get_child_cnt(widget)<2) {
            auto* value=label(widget,"",0,0,&lv_font_montserrat_20);
            lv_obj_set_style_text_opa(value,LV_OPA_COVER,0);lv_obj_clear_flag(value,LV_OBJ_FLAG_CLICKABLE);
        }
    }
    refreshNumericFields();
}
void Ui::refreshNumericFields() {
    for(size_t i=0;i<numeric_binding_count_;i++) {
        const auto& b=numeric_bindings_[i];if(b.slider || !b.widget)continue;
        const float factor=b.decimals==0 ? 1:10;
        char text[64];std::snprintf(text,sizeof(text),"%.*f %s",b.decimals,
            static_cast<double>(lv_spinbox_get_value(b.widget)/factor),b.unit);
        auto* value=lv_obj_get_child(b.widget,-1);if(value) {lv_label_set_text(value,text);lv_obj_center(value);}
    }
}
void Ui::numericOpenEvent(lv_event_t* e) {
    if(instance_)instance_->openNumericEntry(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
}
void Ui::openNumericEntry(size_t index) {
    if(index>=numeric_binding_count_ || auxiliary_screen_ || editor_commit_pending_)return;
    active_numeric_=numeric_bindings_[index];const auto& b=active_numeric_;
    const float value=b.slider ? lv_slider_get_value(b.widget):lv_spinbox_get_value(b.widget)/(b.decimals ? 10.f:1.f);
    numeric_.open(value,b.minimum,b.maximum,b.decimals);
    auxiliary_return_=lv_scr_act();auxiliary_screen_=lv_obj_create(nullptr);styleScreen(auxiliary_screen_);
    label(auxiliary_screen_,b.title,20,14,&lv_font_montserrat_24);
    char range[90];std::snprintf(range,sizeof(range),"Range: %.*f to %.*f %s",b.decimals,static_cast<double>(b.minimum),b.decimals,static_cast<double>(b.maximum),b.unit);
    label(auxiliary_screen_,range,20,53,&lv_font_montserrat_14,UiTheme::muted());
    numeric_text_=label(auxiliary_screen_,numeric_.text(),20,84,&lv_font_montserrat_24);
    lv_obj_set_width(numeric_text_,650);lv_obj_set_style_text_align(numeric_text_,LV_TEXT_ALIGN_CENTER,0);
    if(b.minimum<0)button(auxiliary_screen_,"+ / -",686,74,94,52,numericKeyEvent,'-');
    const char* keys[]={"1","2","3","4","5","6","7","8","9",".","0","<"};
    for(int i=0;i<12;i++) {
        auto* k=button(auxiliary_screen_,keys[i],20+(i%3)*258,142+(i/3)*65,244,56,numericKeyEvent,keys[i][0]);
        lv_obj_set_style_text_font(lv_obj_get_child(k,0),&lv_font_montserrat_24,0);
        if(i==9 && !b.decimals)lv_obj_add_state(k,LV_STATE_DISABLED);
    }
    button(auxiliary_screen_,"CANCEL",20,420,180,48,numericKeyEvent,1001);
    numeric_apply_=button(auxiliary_screen_,"APPLY",590,420,190,48,numericKeyEvent,1000,true);
    update_policy_.activate(UiActivity::TileEditor);refreshNumericEntry();lv_scr_load(auxiliary_screen_);
}
void Ui::refreshNumericEntry() {
    lv_label_set_text(numeric_text_,numeric_.text());
    if(numeric_.valid())lv_obj_clear_state(numeric_apply_,LV_STATE_DISABLED);
    else lv_obj_add_state(numeric_apply_,LV_STATE_DISABLED);
    lv_obj_set_style_text_color(numeric_text_,numeric_.valid() ? UiTheme::text():UiTheme::red(),0);
}
void Ui::numericKeyEvent(lv_event_t* e) {
    if(!instance_)return;auto* s=instance_;
    const auto action=reinterpret_cast<intptr_t>(lv_event_get_user_data(e));
    if(action==1001) {s->closeAuxiliary();return;}
    if(action==1000) {
        if(!s->numeric_.valid())return;
        const auto binding=s->active_numeric_;const float value=s->numeric_.value();
        s->closeAuxiliary();
        if(binding.slider) {lv_slider_set_value(binding.widget,std::lround(value),LV_ANIM_OFF);lv_event_send(binding.widget,LV_EVENT_VALUE_CHANGED,nullptr);}
        else lv_spinbox_set_value(binding.widget,std::lround(value*(binding.decimals ? 10:1)));
        if(s->editor_.isOpen()) {s->syncEditorDraftFromControls();s->refreshEditorPreview();}
        s->refreshNumericFields();return;
    }
    if(action=='<')s->numeric_.erase();else s->numeric_.append(static_cast<char>(action));
    s->refreshNumericEntry();
}
void Ui::closeAuxiliary() {
    if(!auxiliary_screen_)return;
    auto* previous=auxiliary_screen_;auxiliary_screen_=nullptr;
    lv_scr_load(auxiliary_return_);auxiliary_return_=nullptr;
    if(editor_.isOpen())update_policy_.activate(UiActivity::TileEditor);
    else update_policy_.activate(PageId::Settings);
    lv_obj_del_async(previous);numeric_text_=numeric_apply_=nullptr;
}
void Ui::openParameterPicker() {
    if(auxiliary_screen_ || !editor_.isOpen())return;
    syncEditorDraftFromControls();picker_selected_=editor_.draft().tile.parameter;
    picker_category_=static_cast<uint8_t>(parameterCategory(picker_selected_));picker_page_=0;
    auxiliary_return_=editor_screen_;renderParameterPicker();
}
void Ui::renderParameterPicker() {
    auto* previous=auxiliary_screen_;auxiliary_screen_=lv_obj_create(nullptr);styleScreen(auxiliary_screen_);
    label(auxiliary_screen_,"SELECT PARAMETER",20,14,&lv_font_montserrat_24);
    label(auxiliary_screen_,active_source_==DataSource::Demo ? "Available in DEMO":active_profile_ ? active_profile_->id:"No CAN profile",470,22,&lv_font_montserrat_12,UiTheme::muted());
    const char* categories[]={"ENGINE","TEMPERATURE","PRESSURE","FLAGS"};
    for(int i=0;i<4;i++)button(auxiliary_screen_,categories[i],20+i*192,58,182,48,pickerEvent,200+i,i==picker_category_);
    auto options=ParameterOptions::build(active_source_,active_profile_,picker_selected_);
    size_t matched=0,shown=0;
    for(size_t i=0;i<options.count();i++) {
        const auto id=options.parameterAt(i);
        if(static_cast<uint8_t>(parameterCategory(id))!=picker_category_)continue;
        const size_t position=matched++;
        if(position<picker_page_*4 || shown>=4)continue;
        const auto& d=parameterDescriptor(id);char text[110];
        std::snprintf(text,sizeof(text),"%s    %s%s",d.name,UnitPresenter::present(id,0,config_->units).unit,capabilities_.supports(id) ? "":" (UNAVAILABLE)");
        auto* row=button(auxiliary_screen_,text,20,120+shown*58,760,50,pickerEvent,300+static_cast<intptr_t>(id),id==picker_selected_);
        if(!capabilities_.supports(id))lv_obj_add_state(row,LV_STATE_DISABLED);shown++;
    }
    if(!matched)label(auxiliary_screen_,"No parameters available in this category",100,210);
    auto* prev=button(auxiliary_screen_,"<",20,356,90,48,pickerEvent,210);
    auto* next=button(auxiliary_screen_,">",690,356,90,48,pickerEvent,211);
    if(!picker_page_)lv_obj_add_state(prev,LV_STATE_DISABLED);
    if((picker_page_+1)*4>=matched)lv_obj_add_state(next,LV_STATE_DISABLED);
    button(auxiliary_screen_,"BACK",20,420,180,48,pickerEvent,212);
    auto* select=button(auxiliary_screen_,"SELECT",590,420,190,48,pickerEvent,213,true);
    if(!capabilities_.supports(picker_selected_))lv_obj_add_state(select,LV_STATE_DISABLED);
    lv_scr_load(auxiliary_screen_);if(previous)lv_obj_del_async(previous);
}
void Ui::pickerEvent(lv_event_t* e) {
    if(!instance_)return;auto* s=instance_;
    const auto action=reinterpret_cast<intptr_t>(lv_event_get_user_data(e));
    if(action==100) {s->openParameterPicker();return;}
    if(action>=200 && action<204) {s->picker_category_=action-200;s->picker_page_=0;}
    else if(action==210) {if(s->picker_page_)s->picker_page_--;}
    else if(action==211)s->picker_page_++;
    else if(action==212) {s->closeAuxiliary();return;}
    else if(action==213) {
        s->editor_.setParameter(s->picker_selected_);
        for(size_t i=0;i<s->editor_parameter_options_.count();i++)if(s->editor_parameter_options_.parameterAt(i)==s->picker_selected_)lv_dropdown_set_selected(s->editor_parameter_,i);
        s->closeAuxiliary();s->loadEditorControlsFromDraft();return;
    } else if(action>=300)s->picker_selected_=static_cast<ParameterId>(action-300);
    s->renderParameterPicker();
}

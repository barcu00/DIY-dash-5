#include "ui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "ui/editor_widgets.h"
#include "ui/editor_value_model.h"
#include "ui/fonts/numeric_fonts.h"
using namespace EditorWidgets;
namespace {
float fieldValue(lv_obj_t* o) {return lv_spinbox_get_value(o)/10.f;}
void setField(lv_obj_t* o,float v) {lv_spinbox_set_value(o,std::lround(v*10.f));}
}
void Ui::openEditor(TileAddress address) {
    if(!config_ || editor_commit_pending_)return;
    if(editor_screen_)closeEditor();
    editor_return_page_=current_page_;editor_return_category_=settings_flow_.category();
    if(!editor_.open(address,*config_))return;
    if(warning_panel_) {lv_obj_del(warning_panel_);warning_panel_=nullptr;warning_text_=nullptr;}
    numeric_binding_count_=0;editor_tab_=0;
    const auto& tile=editor_.draft().tile;
    editor_screen_=lv_obj_create(nullptr);styleScreen(editor_screen_);
    label(editor_screen_,"TILE SETTINGS",20,12,&lv_font_montserrat_24);
    char context[100];std::snprintf(context,sizeof(context),"%s / SLOT %u",
        address.page==PageId::Dash ? "DASH":"TRACK",address.slot+1);
    editor_context_=label(editor_screen_,context,450,20,&lv_font_montserrat_12,UiTheme::muted());
    const char* tabs[]={"DATA","WARNING","TEMPERATURE BAR"};
    for(int i=0;i<3;i++)editor_tabs_[i]=button(editor_screen_,tabs[i],20+i*254,54,244,48,editorTabEvent,i);
    editor_data_panel_=panel(editor_screen_,0,112,800,298);
    label(editor_data_panel_,"PARAMETER",20,6,&lv_font_montserrat_12,UiTheme::muted());
    // The dropdown remains a compact quick choice; SELECT PARAMETER opens the categorized picker.
    editor_parameter_=lv_dropdown_create(editor_data_panel_);
    darkDropdown(editor_parameter_);
    lv_obj_set_pos(editor_parameter_,20,28);lv_obj_set_size(editor_parameter_,410,48);
    editor_parameter_options_=ParameterOptions::build(active_source_,active_profile_,tile.parameter);
    char options[2048]{};editor_parameter_options_.write(options,sizeof(options));
    lv_dropdown_set_options(editor_parameter_,options);
    for(size_t i=0;i<editor_parameter_options_.count();++i)
        if(editor_parameter_options_.parameterAt(i)==tile.parameter)lv_dropdown_set_selected(editor_parameter_,i);
    lv_obj_add_event_cb(editor_parameter_,editorEvent,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(3));
    button(editor_data_panel_,"SELECT PARAMETER",20,86,410,48,pickerEvent,100);
    editor_visible_=lv_checkbox_create(editor_data_panel_);lv_obj_set_pos(editor_visible_,20,158);
    lv_checkbox_set_text(editor_visible_,"VISIBLE");
    darkCheckbox(editor_visible_);
    editor_decimals_label_=label(editor_data_panel_,"DECIMALS",20,220,&lv_font_montserrat_14,UiTheme::muted());
    editor_decimals_=lv_dropdown_create(editor_data_panel_);lv_obj_set_pos(editor_decimals_,170,208);
    lv_obj_set_size(editor_decimals_,160,48);lv_dropdown_set_options(editor_decimals_,"0\n1\n2\n3");
    lv_obj_add_flag(editor_decimals_,LV_OBJ_FLAG_HIDDEN);
    const char* decimals[]={"0","1","2","3"};
    for(int i=0;i<4;i++)editor_decimal_buttons_[i]=button(editor_data_panel_,decimals[i],170+i*64,208,58,48,editorTabEvent,10+i);
    auto* preview=panel(editor_data_panel_,470,6,306,270);
    lv_obj_set_style_border_width(preview,1,0);lv_obj_set_style_border_color(preview,UiTheme::border(),0);
    editor_preview_name_=label(preview,"",0,22,&lv_font_montserrat_20);lv_obj_set_width(editor_preview_name_,306);
    lv_obj_set_style_text_align(editor_preview_name_,LV_TEXT_ALIGN_CENTER,0);
    editor_preview_value_=label(preview,"",0,74,&race_digits_64);lv_obj_set_width(editor_preview_value_,306);
    lv_obj_set_style_text_align(editor_preview_value_,LV_TEXT_ALIGN_CENTER,0);
    editor_preview_unit_=label(preview,"",0,156,&lv_font_montserrat_20);lv_obj_set_width(editor_preview_unit_,306);
    lv_obj_set_style_text_align(editor_preview_unit_,LV_TEXT_ALIGN_CENTER,0);
    editor_preview_bar_=lv_bar_create(preview);lv_obj_set_pos(editor_preview_bar_,20,224);lv_obj_set_size(editor_preview_bar_,266,14);
    lv_bar_set_range(editor_preview_bar_,0,1000);
    editor_numeric_panel_=panel(editor_screen_,0,112,800,298);
    editor_warning_=lv_checkbox_create(editor_numeric_panel_);lv_obj_set_pos(editor_warning_,20,12);
    lv_checkbox_set_text(editor_warning_,"ENABLE WARNING");
    darkCheckbox(editor_warning_);
    label(editor_numeric_panel_,"TRIGGER",20,72,&lv_font_montserrat_14,UiTheme::muted());
    editor_direction_=lv_dropdown_create(editor_numeric_panel_);lv_obj_set_pos(editor_direction_,180,58);
    darkDropdown(editor_direction_);
    lv_obj_set_size(editor_direction_,270,48);lv_dropdown_set_options(editor_direction_,"ABOVE\nBELOW");
    lv_obj_add_event_cb(editor_direction_,editorTabEvent,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(4));
    label(editor_numeric_panel_,"THRESHOLD",20,132,&lv_font_montserrat_14,UiTheme::muted());
    editor_threshold_=field(editor_numeric_panel_,180,118,270,0,999999,0);
    editor_summary_=label(editor_numeric_panel_,"",480,66,&lv_font_montserrat_14,UiTheme::text());lv_obj_set_width(editor_summary_,294);
    editor_reset_label_=label(editor_numeric_panel_,"RESET BELOW",20,192,&lv_font_montserrat_14,UiTheme::muted());
    editor_hysteresis_=field(editor_numeric_panel_,180,178,270,-9999999,9999999,0,7,6);
    label(editor_numeric_panel_,"DELAY (s)",20,252,&lv_font_montserrat_14,UiTheme::muted());
    editor_delay_=field(editor_numeric_panel_,180,238,270,0,100,0,3,2);
    button(editor_numeric_panel_,"TEST WARNING",480,188,294,52,warningTestEvent,0);
    editor_temperature_panel_=panel(editor_screen_,0,112,800,298);
    editor_temperature_bar_=lv_checkbox_create(editor_temperature_panel_);lv_obj_set_pos(editor_temperature_bar_,20,12);
    lv_checkbox_set_text(editor_temperature_bar_,"ENABLE TEMPERATURE BAR");
    darkCheckbox(editor_temperature_bar_);
    const char* names[]={"MIN","READY","RED","MAX"};lv_obj_t** fields[]={&editor_temperature_minimum_,&editor_temperature_ready_,&editor_temperature_red_,&editor_temperature_maximum_};
    for(int i=0;i<4;i++) {label(editor_temperature_panel_,names[i],20+i*194,72,&lv_font_montserrat_14,UiTheme::muted());
        *fields[i]=field(editor_temperature_panel_,20+i*194,100,176,-999999,999999,0);}
    const lv_color_t zone_colors[]={UiTheme::blue(),UiTheme::green(),UiTheme::red()};
    for(int i=0;i<3;i++) {editor_temperature_zones_[i]=panel(editor_temperature_panel_,20,182,1,16);
        lv_obj_set_style_bg_color(editor_temperature_zones_[i],zone_colors[i],0);}
    editor_temperature_marker_=panel(editor_temperature_panel_,20,174,3,32);
    lv_obj_set_style_bg_color(editor_temperature_marker_,UiTheme::text(),0);
    editor_temperature_value_=label(editor_temperature_panel_,"",20,156,&lv_font_montserrat_12);
    label(editor_temperature_panel_,"Cold below READY | Normal below RED | Hot at RED",20,224,&lv_font_montserrat_14);
    label(editor_temperature_panel_,"Order: MIN < READY < RED <= MAX",20,266,&lv_font_montserrat_12,UiTheme::muted());
    editor_flag_panel_=panel(editor_screen_,0,112,800,298);
    label(editor_flag_panel_,"ACTIVE COLOR",230,50,&lv_font_montserrat_20);
    editor_flag_color_=lv_dropdown_create(editor_flag_panel_);lv_obj_set_pos(editor_flag_color_,230,100);
    lv_obj_set_size(editor_flag_color_,340,54);lv_dropdown_set_options(editor_flag_color_,"YELLOW\nGREEN\nRED");
    darkDropdown(editor_flag_color_);
    label(editor_flag_panel_,"OFF stays neutral. ON uses the selected color.",160,192);
    editor_cancel_=button(editor_screen_,"CANCEL",20,420,180,48,editorEvent,1);
    editor_save_=button(editor_screen_,"SAVE TILE",590,420,190,48,editorEvent,2,true);
    editor_message_=label(editor_screen_,"Unsaved changes",225,436,&lv_font_montserrat_14,UiTheme::muted());
    loadEditorControlsFromDraft();
    update_policy_.activate(UiActivity::TileEditor);lv_scr_load(editor_screen_);
}
void Ui::showEditorTab(uint8_t tab) {
    if(!editor_.isOpen())return;
    const auto& descriptor=parameterDescriptor(editor_.draft().tile.parameter);
    const bool flag=descriptor.kind==ParameterKind::Flag;
    const bool temperature=descriptor.native_unit==NativeUnit::Celsius;
    if(tab==2 && !temperature)tab=0;
    editor_tab_=tab;
    lv_obj_t* panels[]={editor_data_panel_,editor_numeric_panel_,editor_temperature_panel_,editor_flag_panel_};
    for(auto* p:panels)lv_obj_add_flag(p,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(tab==0 ? editor_data_panel_:tab==2 ? editor_temperature_panel_:flag ? editor_flag_panel_:editor_numeric_panel_,LV_OBJ_FLAG_HIDDEN);
    for(int i=0;i<3;i++) {
        lv_obj_set_style_bg_color(editor_tabs_[i],UiTheme::background(),0);
        lv_obj_set_style_border_side(editor_tabs_[i],LV_BORDER_SIDE_BOTTOM,0);
        lv_obj_set_style_border_width(editor_tabs_[i],i==tab ? 3:1,0);
        lv_obj_set_style_border_color(editor_tabs_[i],i==tab ? UiTheme::blue():UiTheme::border(),0);
        auto* caption=lv_obj_get_child(editor_tabs_[i],0);
        if(i==1)lv_label_set_text(caption,flag ? "ACTIVE COLOR":"WARNING");
        lv_obj_set_style_text_color(caption,i==tab ? UiTheme::blue():UiTheme::text(),0);
    }
    if(temperature)lv_obj_clear_flag(editor_tabs_[2],LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(editor_tabs_[2],LV_OBJ_FLAG_HIDDEN);
    if(flag) {lv_obj_add_flag(editor_decimals_,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(editor_decimals_label_,LV_OBJ_FLAG_HIDDEN);}
    else lv_obj_clear_flag(editor_decimals_label_,LV_OBJ_FLAG_HIDDEN);
    for(auto* b:editor_decimal_buttons_) {
        if(flag)lv_obj_add_flag(b,LV_OBJ_FLAG_HIDDEN);else lv_obj_clear_flag(b,LV_OBJ_FLAG_HIDDEN);
    }
    refreshEditorPreview();
}
void Ui::editorTabEvent(lv_event_t* event) {
    if(!instance_ || !instance_->editor_.isOpen())return;
    auto* self=instance_;const auto action=reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    if(action>=10 && action<=13) {
        lv_dropdown_set_selected(self->editor_decimals_,action-10);
        self->syncEditorDraftFromControls();self->refreshEditorPreview();return;
    }
    if(action==4) {
        const auto requested=static_cast<WarningDirection>(lv_dropdown_get_selected(self->editor_direction_));
        lv_dropdown_set_selected(self->editor_direction_,static_cast<uint16_t>(self->editor_.draft().tile.warning.direction));
        self->syncEditorDraftFromControls();
        if(!self->editor_controls_valid_)return;
        auto warning=self->editor_.draft().tile.warning;
        warning.direction=requested;
        self->editor_.setWarning(warning);self->loadEditorControlsFromDraft();return;
    }
    self->syncEditorDraftFromControls();self->showEditorTab(action);
}
void Ui::refreshEditorParameterControls(ParameterId) {showEditorTab(editor_tab_);}
void Ui::loadEditorTemperatureControls(const TemperatureBarConfig& bar) {
    const auto id=editor_.draft().tile.parameter;
    if(bar.enabled)lv_obj_add_state(editor_temperature_bar_,LV_STATE_CHECKED);else lv_obj_clear_state(editor_temperature_bar_,LV_STATE_CHECKED);
    lv_obj_t* fields[]={editor_temperature_minimum_,editor_temperature_ready_,editor_temperature_red_,editor_temperature_maximum_};
    const float values[]={bar.minimum_native,bar.ready_native,bar.red_native,bar.maximum_native};
    for(int i=0;i<4;i++)setField(fields[i],UnitPresenter::present(id,values[i],config_->units).value);
}
void Ui::loadEditorControlsFromDraft() {
    const auto& t=editor_.draft().tile;const auto id=t.parameter;
    if(t.visible)lv_obj_add_state(editor_visible_,LV_STATE_CHECKED);else lv_obj_clear_state(editor_visible_,LV_STATE_CHECKED);
    lv_dropdown_set_selected(editor_decimals_,t.decimals);lv_dropdown_set_selected(editor_flag_color_,static_cast<uint16_t>(t.flag_active_color));
    if(t.warning.enabled)lv_obj_add_state(editor_warning_,LV_STATE_CHECKED);else lv_obj_clear_state(editor_warning_,LV_STATE_CHECKED);
    lv_dropdown_set_selected(editor_direction_,static_cast<uint16_t>(t.warning.direction));
    const auto present=[&](float v){return UnitPresenter::present(id,v,config_->units).value;};
    setField(editor_threshold_,present(t.warning.threshold_native));
    setField(editor_hysteresis_,present(t.warning.threshold_native+(t.warning.direction==WarningDirection::Above ? -t.warning.hysteresis_native:t.warning.hysteresis_native)));
    setField(editor_delay_,t.warning.delay_ms/1000.f);loadEditorTemperatureControls(t.temperature_bar);
    numeric_binding_count_=0;const char* unit=UnitPresenter::present(id,0,config_->units).unit;
    bindNumeric(editor_threshold_,"WARNING THRESHOLD",present(0),present(999),1,unit);
    bindNumeric(editor_hysteresis_,t.warning.direction==WarningDirection::Above ? "RESET BELOW":"RESET ABOVE",present(-999),present(1998),1,unit);
    bindNumeric(editor_delay_,"WARNING DELAY",0,10,1,"s");
    const char* names[]={"TEMPERATURE MIN","TEMPERATURE READY","TEMPERATURE RED","TEMPERATURE MAX"};
    lv_obj_t* fields[]={editor_temperature_minimum_,editor_temperature_ready_,editor_temperature_red_,editor_temperature_maximum_};
    for(int i=0;i<4;i++)bindNumeric(fields[i],names[i],present(-999),present(999),1,unit);
    showEditorTab(editor_tab_);
}
void Ui::syncEditorDraftFromControls() {
    if(!editor_.isOpen())return;
    editor_controls_valid_=true;
    editor_.setVisible(lv_obj_has_state(editor_visible_,LV_STATE_CHECKED));
    editor_.setFlagActiveColor(static_cast<FlagActiveColor>(lv_dropdown_get_selected(editor_flag_color_)));
    const auto id=editor_.draft().tile.parameter;
    if(parameterDescriptor(id).kind==ParameterKind::Flag)return;
    editor_.setDecimals(lv_dropdown_get_selected(editor_decimals_));
    const auto old_warning=editor_.draft().tile.warning;
    const auto presented=[&](float v){return UnitPresenter::present(id,v,config_->units).value;};
    const bool threshold_changed=lv_spinbox_get_value(editor_threshold_)!=std::lround(presented(old_warning.threshold_native)*10);
    const float old_reset=old_warning.threshold_native+(old_warning.direction==WarningDirection::Above ? -old_warning.hysteresis_native:old_warning.hysteresis_native);
    const bool reset_changed=lv_spinbox_get_value(editor_hysteresis_)!=std::lround(presented(old_reset)*10);
    const uint16_t delay=lv_spinbox_get_value(editor_delay_)==std::lround(old_warning.delay_ms/100.f)
        ? old_warning.delay_ms:static_cast<uint16_t>(std::lround(fieldValue(editor_delay_)*1000));
    TileWarningConfig warning=old_warning;
    bool warning_valid=true;
    if(!threshold_changed && !reset_changed) {
        warning.enabled=lv_obj_has_state(editor_warning_,LV_STATE_CHECKED);
        warning.delay_ms=delay;
    } else warning_valid=warningFromPresented(id,config_->units,lv_obj_has_state(editor_warning_,LV_STATE_CHECKED),
        static_cast<WarningDirection>(lv_dropdown_get_selected(editor_direction_)),fieldValue(editor_threshold_),fieldValue(editor_hysteresis_),
        delay,warning);
    if(!warning_valid) {
        editor_controls_valid_=false;lv_label_set_text(editor_message_,"Check threshold / reset value");
    } else editor_.setWarning(warning);
    if(parameterDescriptor(id).native_unit==NativeUnit::Celsius) {
        const auto old_bar=editor_.draft().tile.temperature_bar;
        const auto native=[&](lv_obj_t* field,float original) {
            return lv_spinbox_get_value(field)==std::lround(presented(original)*10)
                ? original:UnitPresenter::toNative(id,fieldValue(field),config_->units);
        };
        TemperatureBarConfig bar;bar.enabled=lv_obj_has_state(editor_temperature_bar_,LV_STATE_CHECKED);
        bar.minimum_native=native(editor_temperature_minimum_,old_bar.minimum_native);
        bar.ready_native=native(editor_temperature_ready_,old_bar.ready_native);
        bar.red_native=native(editor_temperature_red_,old_bar.red_native);
        bar.maximum_native=native(editor_temperature_maximum_,old_bar.maximum_native);
        editor_.setTemperatureBar(bar);
        if(!(bar.minimum_native<bar.ready_native && bar.ready_native<bar.red_native && bar.red_native<=bar.maximum_native)) {
            editor_controls_valid_=false;lv_label_set_text(editor_message_,"Require MIN < READY < RED <= MAX");
        }
    }
}
void Ui::refreshEditorPreview() {
    const auto& t=editor_.draft().tile;const auto& d=parameterDescriptor(t.parameter);
    lv_label_set_text(editor_preview_name_,d.short_name);
    char context[100];std::snprintf(context,sizeof(context),"%s / %s",editor_.draft().address.page==PageId::Dash ? "DASH":"TRACK",d.name);
    lv_label_set_text(editor_context_,context);
    for(int i=0;i<4;i++) {
        lv_obj_set_style_bg_color(editor_decimal_buttons_[i],i==t.decimals ? UiTheme::blue():UiTheme::background(),0);
        lv_obj_set_style_text_color(lv_obj_get_child(editor_decimal_buttons_[i],0),i==t.decimals ? UiTheme::background():UiTheme::text(),0);
    }
    const auto& signal=latest_state_.get(t.parameter);const auto value=UnitPresenter::present(t.parameter,signal.value,config_->units);
    char text[64]="---";
    if(signal.valid && capabilities_.supports(t.parameter)) {
        if(d.kind==ParameterKind::Flag)std::snprintf(text,sizeof(text),"%s",signal.value!=0 ? "ON":"OFF");
        else std::snprintf(text,sizeof(text),"%.*f",t.decimals,static_cast<double>(value.value));
    }
    lv_label_set_text(editor_preview_value_,text);lv_label_set_text(editor_preview_unit_,value.unit);
    if(d.native_unit==NativeUnit::Celsius && t.temperature_bar.enabled) {
        lv_obj_clear_flag(editor_preview_bar_,LV_OBJ_FLAG_HIDDEN);
        const float span=t.temperature_bar.maximum_native-t.temperature_bar.minimum_native;
        const int fill=span>0 ? std::clamp<int>((signal.value-t.temperature_bar.minimum_native)*1000/span,0,1000):0;
        lv_bar_set_value(editor_preview_bar_,fill,LV_ANIM_OFF);
        lv_obj_set_style_bg_color(editor_preview_bar_,signal.value>=t.temperature_bar.red_native ? UiTheme::red():signal.value>=t.temperature_bar.ready_native ? UiTheme::green():UiTheme::blue(),LV_PART_INDICATOR);
    } else lv_obj_add_flag(editor_preview_bar_,LV_OBJ_FLAG_HIDDEN);
    std::snprintf(text,sizeof(text),"%s\nThreshold: %.1f %s\nReset: %.1f %s",
        lv_dropdown_get_selected(editor_direction_)==0 ? "RESET BELOW":"RESET ABOVE",
        static_cast<double>(fieldValue(editor_threshold_)),value.unit,static_cast<double>(fieldValue(editor_hysteresis_)),value.unit);
    lv_label_set_text(editor_summary_,text);refreshNumericFields();
    lv_label_set_text(editor_reset_label_,lv_dropdown_get_selected(editor_direction_)==0 ? "RESET BELOW":"RESET ABOVE");
    if(d.native_unit==NativeUnit::Celsius) {
        const auto& b=t.temperature_bar;const float span=b.maximum_native-b.minimum_native;
        if(span>0 && b.minimum_native<b.ready_native && b.ready_native<b.red_native && b.red_native<=b.maximum_native) {
            const int edges[]={0,static_cast<int>((b.ready_native-b.minimum_native)*760/span),static_cast<int>((b.red_native-b.minimum_native)*760/span),760};
            for(int i=0;i<3;i++) {lv_obj_set_pos(editor_temperature_zones_[i],20+edges[i],182);lv_obj_set_width(editor_temperature_zones_[i],std::max(1,edges[i+1]-edges[i]));}
            const int marker=std::clamp<int>((signal.value-b.minimum_native)*760/span,0,757);
            lv_obj_set_pos(editor_temperature_marker_,20+marker,174);
            char temp[24];std::snprintf(temp,sizeof(temp),"%.1f %s",static_cast<double>(value.value),value.unit);
            lv_label_set_text(editor_temperature_value_,temp);lv_obj_set_pos(editor_temperature_value_,std::clamp(20+marker-26,20,700),156);
        }
        if(signal.valid) {lv_obj_clear_flag(editor_temperature_marker_,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(editor_temperature_value_,LV_OBJ_FLAG_HIDDEN);}
        else {lv_obj_add_flag(editor_temperature_marker_,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(editor_temperature_value_,LV_OBJ_FLAG_HIDDEN);}
    }
}
void Ui::closeEditor() {
    if(auxiliary_screen_)closeAuxiliary();
    if(!editor_screen_) {editor_.cancel();return;}
    auto* previous=editor_screen_;editor_screen_=nullptr;editor_cancel_=editor_save_=nullptr;
    editor_.cancel();numeric_binding_count_=0;
    if(editor_return_page_==Page::Settings) {current_page_=Page::Settings;update_policy_.activate(PageId::Settings);showSettings(editor_return_category_);}
    else load(editor_return_page_);
    lv_obj_del_async(previous);
}
void Ui::saveEditor() {
    if(!config_ || !editor_.isOpen() || editor_commit_pending_)return;
    syncEditorDraftFromControls();if(!editor_controls_valid_)return;
    AppConfig candidate=*config_;
    if(!editor_.writeCandidate(candidate)) {lv_label_set_text(editor_message_,"Check value ranges");return;}
    commit_model_.markDirty(false);
    if(!commit_model_.queueOnExit(candidate)) {lv_label_set_text(editor_message_,"SAVE FAILED");return;}
    editor_commit_pending_=true;lv_label_set_text(editor_message_,"SAVING");
    lv_obj_add_state(editor_cancel_,LV_STATE_DISABLED);lv_obj_add_state(editor_save_,LV_STATE_DISABLED);
}
bool Ui::takeWarningTest() {const bool result=warning_test_requested_;warning_test_requested_=false;return result;}
void Ui::warningTestEvent(lv_event_t* e) {
    if(!instance_)return;auto* s=instance_;
    if(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))==1) {
        auto* previous=s->warning_test_screen_;s->warning_test_screen_=nullptr;
        lv_scr_load(s->editor_screen_);lv_obj_del_async(previous);return;
    }
    s->warning_test_requested_=true;s->warning_test_screen_=lv_obj_create(nullptr);styleScreen(s->warning_test_screen_);
    lv_obj_set_style_bg_color(s->warning_test_screen_,UiTheme::red(),0);
    label(s->warning_test_screen_,"WARNING TEST",160,70,&lv_font_montserrat_24);
    label(s->warning_test_screen_,"Preview only - actual alarms are unchanged",100,135,&lv_font_montserrat_20);
    char value[90];std::snprintf(value,sizeof(value),"%s: %.1f %s",parameterDescriptor(s->editor_.draft().tile.parameter).short_name,
        static_cast<double>(fieldValue(s->editor_threshold_)),UnitPresenter::present(s->editor_.draft().tile.parameter,0,s->config_->units).unit);
    label(s->warning_test_screen_,value,100,215,&lv_font_montserrat_24);
    button(s->warning_test_screen_,"CLOSE TEST",200,350,400,64,warningTestEvent,1);
    lv_scr_load(s->warning_test_screen_);
}

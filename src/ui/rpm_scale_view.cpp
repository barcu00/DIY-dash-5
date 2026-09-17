#include "rpm_scale_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "ui/dashboard_layout.h"
#include "ui/ui_theme.h"
#include "ui/fonts/numeric_fonts.h"

namespace {
constexpr float kPi=3.14159265358979323846f;
constexpr float kStripRadius=1196.f;
float stripSpan() { return 2.f*std::asin(379.f/kStripRadius)*180.f/kPi; }
lv_point_t point(int x,int y) { return {static_cast<lv_coord_t>(x),static_cast<lv_coord_t>(y)}; }
lv_point_t polar(lv_point_t center,float radius,float angle) {
    const float a=angle*kPi/180.f;
    return point(std::lround(center.x+radius*std::cos(a)),std::lround(center.y+radius*std::sin(a)));
}
void line(lv_draw_ctx_t* ctx,lv_point_t a,lv_point_t b,lv_color_t color,int width=2) {
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);
    d.color=color;d.width=width;d.round_start=d.round_end=1;lv_draw_line(ctx,&d,&a,&b);
}
void arc(lv_draw_ctx_t* ctx,lv_point_t center,int radius,int start,int end,lv_color_t color,int width) {
    lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);
    d.color=color;d.width=width;d.rounded=0;lv_draw_arc(ctx,&d,&center,radius,start,end);
}
void label(lv_draw_ctx_t* ctx,int x,int y,const char* text,const lv_font_t* font=&lv_font_montserrat_16,
           lv_color_t color=UiTheme::text()) {
    lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.color=color;d.font=font;d.align=LV_TEXT_ALIGN_CENTER;
    const int half=std::strlen(text)>6 ? 80:30;
    lv_area_t area{static_cast<lv_coord_t>(x-half),static_cast<lv_coord_t>(y),
        static_cast<lv_coord_t>(x+half),static_cast<lv_coord_t>(y+font->line_height+2)};
    lv_draw_label(ctx,&d,&area,text,nullptr);
}
lv_color_t zone(float rpm,uint16_t yellow,uint16_t red) {
    return rpm>=red ? UiTheme::red():rpm>=yellow ? UiTheme::yellow():UiTheme::green();
}
bool dial(DashboardLayout layout) {
    return layout==DashboardLayout::AnalogStyle || layout==DashboardLayout::ModernMotorsport;
}
void transparent(lv_obj_t* obj) {
    lv_obj_set_style_bg_opa(obj,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(obj,0,0);lv_obj_set_style_pad_all(obj,0,0);
    lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
}
} // namespace

void RpmScaleView::create(lv_obj_t* parent) {
    root_=lv_obj_create(parent);transparent(root_);lv_obj_add_event_cb(root_,drawScale,LV_EVENT_DRAW_MAIN,this);
    indicator_=lv_obj_create(root_);transparent(indicator_);lv_obj_add_event_cb(indicator_,drawIndicator,LV_EVENT_DRAW_MAIN,this);
    value_=lv_label_create(root_);lv_obj_set_style_text_font(value_,&race_digits_96,0);
    lv_obj_set_style_text_color(value_,UiTheme::text(),0);lv_obj_set_style_text_align(value_,LV_TEXT_ALIGN_CENTER,0);
    lv_label_set_long_mode(value_,LV_LABEL_LONG_CLIP);
    caption_=lv_label_create(root_);lv_label_set_text(caption_,"RPM");
    lv_obj_set_style_text_font(caption_,&lv_font_montserrat_20,0);lv_obj_set_style_text_color(caption_,UiTheme::muted(),0);
    lv_obj_add_flag(root_,LV_OBJ_FLAG_HIDDEN);
}
void RpmScaleView::apply(DashboardLayout layout,uint16_t maximum) {
    layout_=layout;maximum_=normalizedRpmScale(maximum);fill_=0;
    valid_=flashing_=red_phase_=initialized_=false;last_text_.fill('\0');filter_.reset();
    if(!dial(layout) && layout!=DashboardLayout::SideGear && layout!=DashboardLayout::StripStyle) {
        lv_obj_add_flag(root_,LV_OBJ_FLAG_HIDDEN);return;
    }
    lv_obj_clear_flag(root_,LV_OBJ_FLAG_HIDDEN);
    const bool side=layout==DashboardLayout::SideGear;
    lv_obj_set_pos(root_,side ? 152:8,side ? 16:8);lv_obj_set_size(root_,dial(layout) ? 448:side ? 640:784,side ? 292:414);
    lv_obj_set_style_border_width(root_,side ? 1:0,0);lv_obj_set_style_border_color(root_,UiTheme::border(),0);lv_obj_set_style_radius(root_,7,0);
    lv_obj_set_pos(indicator_,side ? 16:0,side ? 50:0);lv_obj_set_size(indicator_,dial(layout) ? 448:side ? 608:784,dial(layout) ? 414:side ? 30:150);
    if(dial(layout)) {
        lv_obj_clear_flag(value_,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(caption_,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(value_,280);lv_label_set_text(value_,"---");const bool e=layout==DashboardLayout::ModernMotorsport;
        lv_obj_align(value_,LV_ALIGN_TOP_MID,-4,e ? 174:166);lv_obj_align(caption_,LV_ALIGN_TOP_MID,-4,e ? 274:263);
        lv_obj_set_style_text_color(value_,UiTheme::text(),0);
    } else { lv_obj_add_flag(value_,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(caption_,LV_OBJ_FLAG_HIDDEN); }
    if(layout==DashboardLayout::StripStyle) {
        const float span=stripSpan(),start=270-span/2,step=span/36;
        // Congruent radial sectors, no slant. Chord sag is below 0.05px;
        // cache once instead of the prototype's per-pixel rasterisation.
        for(int i=0;i<36;i++) {
            const float a=start+(i+.1f)*step,b=start+(i+.9f)*step;
            strip_blocks_[i]={polar(point(392,1226),kStripRadius,a),polar(point(392,1226),kStripRadius,b),
                polar(point(392,1226),kStripRadius-28,b),polar(point(392,1226),kStripRadius-28,a)};
        }
    }
    lv_obj_invalidate(root_);
}
lv_point_t RpmScaleView::needleEnd(uint16_t fill) const {
    lv_area_t a;lv_obj_get_coords(indicator_,&a);return polar(point(a.x1+220,a.y1+212),190,135+270*fill/1000.f);
}
void RpmScaleView::invalidateNeedle(uint16_t fill) {
    const auto p=needleEnd(fill);
    lv_area_t dirty{static_cast<lv_coord_t>(p.x-24),static_cast<lv_coord_t>(p.y-24),static_cast<lv_coord_t>(p.x+24),static_cast<lv_coord_t>(p.y+24)};
    lv_obj_invalidate_area(indicator_,&dirty);
}
void RpmScaleView::invalidateArc(uint16_t previous,uint16_t current) {
    lv_area_t a;lv_obj_get_coords(indicator_,&a);const auto center=point(a.x1+220,a.y1+272);
    int x1=a.x2,y1=a.y2,x2=a.x1,y2=a.y1;
    const int lo=static_cast<int>(std::floor(180+180*std::min(previous,current)/1000.f));
    const int hi=static_cast<int>(std::ceil(180+180*std::max(previous,current)/1000.f));
    for(int angle=lo;angle<=hi;angle++)for(int r:{191,219}) {
        const auto p=polar(center,r,angle);
        x1=std::min(x1,static_cast<int>(p.x));x2=std::max(x2,static_cast<int>(p.x));
        y1=std::min(y1,static_cast<int>(p.y));y2=std::max(y2,static_cast<int>(p.y));
    }
    lv_area_t dirty{static_cast<lv_coord_t>(x1-5),static_cast<lv_coord_t>(y1-5),static_cast<lv_coord_t>(x2+5),static_cast<lv_coord_t>(y2+5)};
    lv_obj_invalidate_area(indicator_,&dirty);
}
void RpmScaleView::invalidateBlocks(uint16_t previous,uint16_t current) {
    lv_area_t a;lv_obj_get_coords(indicator_,&a);
    if(layout_!=DashboardLayout::StripStyle) {
        const int width=lv_obj_get_width(indicator_)-8;
        a.x2=a.x1+4+width*std::max(previous,current)/1000+9;a.x1+=std::max(0,4+width*std::min(previous,current)/1000-5);
    } else {
        const int lo=std::clamp<int>(std::min(previous,current)*36/1000,0,35),hi=std::clamp<int>(std::max(previous,current)*36/1000,0,35);
        int x1=784,x2=0,y1=150,y2=0;
        for(int i=lo;i<=hi;i++)for(const auto& p:strip_blocks_[i]) {
            x1=std::min(x1,static_cast<int>(p.x));x2=std::max(x2,static_cast<int>(p.x));
            y1=std::min(y1,static_cast<int>(p.y));y2=std::max(y2,static_cast<int>(p.y));
        }
        a={static_cast<lv_coord_t>(a.x1+x1-3),static_cast<lv_coord_t>(a.y1+y1-3),static_cast<lv_coord_t>(a.x1+x2+3),static_cast<lv_coord_t>(a.y1+y2+3)};
    }
    lv_obj_invalidate_area(indicator_,&a);
}
void RpmScaleView::update(const SignalValue& raw,uint32_t now_ms,const ShiftLightConfig& shift) {
    if(lv_obj_has_flag(root_,LV_OBJ_FLAG_HIDDEN))return;
    if(yellow_from_!=shift.start_rpm || red_from_!=shift.red_rpm) {
        yellow_from_=shift.start_rpm;red_from_=shift.red_rpm;lv_obj_invalidate(root_);
    }
    const bool flashing=raw.valid && shift.flash_enabled && raw.value>=shift.flash_rpm,red=flashing && now_ms%250U<125U;
    const bool due=!initialized_ || now_ms-last_update_ms_>=25U || raw.valid!=valid_;
    if(!due && flashing==flashing_ && red==red_phase_)return;
    const auto rpm=filter_.sample(raw,now_ms,50U);const auto fill=rpmScaleFill(rpm.value,rpm.valid,maximum_);
    if(!initialized_ || rpm.valid!=valid_ || fill!=fill_ || flashing!=flashing_ || red!=red_phase_) {
        if(layout_==DashboardLayout::AnalogStyle) { invalidateNeedle(fill_);invalidateNeedle(fill); }
        else if(!initialized_ || rpm.valid!=valid_ || flashing!=flashing_ || red!=red_phase_)lv_obj_invalidate(indicator_);
        else if(layout_==DashboardLayout::ModernMotorsport)invalidateArc(fill_,fill);
        else invalidateBlocks(fill_,fill);
        if(dial(layout_))lv_obj_set_style_text_color(value_,red ? UiTheme::red():UiTheme::text(),0);
        fill_=fill;valid_=rpm.valid;flashing_=flashing;red_phase_=red;
    }
    if(dial(layout_) && due) {
        char text[16]="---";if(rpm.valid)std::snprintf(text,sizeof(text),"%.0f",static_cast<double>(rpm.value));
        if(std::strcmp(text,last_text_.data())) { lv_label_set_text(value_,text);std::snprintf(last_text_.data(),last_text_.size(),"%s",text); }
    }
    if(due)last_update_ms_=now_ms;initialized_=true;
}
void RpmScaleView::drawScale(lv_event_t* event) {
    auto* self=static_cast<RpmScaleView*>(lv_event_get_user_data(event));auto* ctx=lv_event_get_draw_ctx(event);
    lv_area_t a,root;lv_obj_get_coords(self->indicator_,&a);lv_obj_get_coords(self->root_,&root);
    const bool analog=self->layout_==DashboardLayout::AnalogStyle,e=self->layout_==DashboardLayout::ModernMotorsport,curved=self->layout_==DashboardLayout::StripStyle;
    const auto center=point(a.x1+220,a.y1+(e ? 272:212)),strip_center=point(a.x1+392,a.y1+1226);
    const float span=stripSpan(),start=270-span/2;
    if(analog) {
        arc(ctx,center,207,135,405,UiTheme::border(),1);
        for(int i=0;i<54;i++)arc(ctx,center,202,135+i*5,139+i*5,zone(self->maximum_*(i+.5f)/54,self->yellow_from_,self->red_from_),10);
    } else if(e) {
        arc(ctx,center,219,180,360,UiTheme::border(),1);arc(ctx,center,214,180,360,lv_color_hex(0x183A4B),1);
        line(ctx,point(root.x1+72,root.y1+318),point(root.x1+368,root.y1+318),lv_color_hex(0x183A4B),1);
        line(ctx,point(root.x1+196,root.y1+318),point(root.x1+244,root.y1+318),UiTheme::blue(),2);
    } else if(curved) {
        arc(ctx,strip_center,1214,251,289,UiTheme::blue(),1);
        line(ctx,point(root.x1+392,root.y1+210),point(root.x1+392,root.y1+396),UiTheme::border(),1);
    }
    for(int rpm=0;rpm<=self->maximum_;rpm+=200) {
        const float f=rpm/static_cast<float>(self->maximum_);const bool major=rpm%1000==0;
        char text[8];std::snprintf(text,sizeof(text),"%d",rpm/1000);
        if(analog || e) {
            const float angle=e ? 180+180*f:135+270*f;
            line(ctx,polar(center,187,angle),polar(center,major ? (e ? 176:174):181,angle),major ? UiTheme::text():UiTheme::muted(),major ? 2:1);
            if(major) { auto p=polar(center,e ? 159:153,angle);if(analog)p.x+=rpm==0 ? -24:rpm==self->maximum_ ? 24:0;label(ctx,p.x,p.y-12,text,&lv_font_montserrat_24); }
        } else if(curved) {
            const float angle=start+span*f;
            line(ctx,polar(strip_center,kStripRadius+4,angle),polar(strip_center,kStripRadius+(major ? 14:9),angle),zone(rpm,self->yellow_from_,self->red_from_),major ? 2:1);
            if(major) { const auto p=polar(strip_center,kStripRadius-49,angle);label(ctx,p.x,p.y-8,text); }
        } else {
            const int x=a.x1+4+static_cast<int>((lv_obj_get_width(self->indicator_)-8)*f);
            line(ctx,point(x,a.y1-10),point(x,a.y1-4),zone(rpm,self->yellow_from_,self->red_from_),major ? 2:1);
            if(major)label(ctx,std::clamp(x,static_cast<int>(a.x1+14),static_cast<int>(a.x2-18)),a.y1+43,text);
        }
    }
    if(analog || e)label(ctx,center.x,root.y1+(e ? 142:93),"RPM x1000",&lv_font_montserrat_16,UiTheme::muted());
    else label(ctx,curved ? root.x2-72:root.x1+88,root.y1+(curved ? -2:12),"RPM x1000",&lv_font_montserrat_16,UiTheme::muted());
}
void RpmScaleView::drawIndicator(lv_event_t* event) {
    auto* self=static_cast<RpmScaleView*>(lv_event_get_user_data(event));auto* ctx=lv_event_get_draw_ctx(event);
    lv_area_t a;lv_obj_get_coords(self->indicator_,&a);const auto dim=lv_color_hex(0x151D22);
    if(self->layout_==DashboardLayout::AnalogStyle) {
        if(!self->valid_)return;
        const auto center=point(a.x1+220,a.y1+212);const float angle=135+270*self->fill_/1000.f,radians=angle*kPi/180.f;
        const auto out=polar(center,205,angle),in=polar(center,175,angle);const float tx=-std::sin(radians)*5,ty=std::cos(radians)*5;
        const lv_point_t p[]={point(std::lround(out.x+tx),std::lround(out.y+ty)),point(std::lround(out.x-tx),std::lround(out.y-ty)),
            point(std::lround(in.x-tx),std::lround(in.y-ty)),point(std::lround(in.x+tx),std::lround(in.y+ty))};
        lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=self->flashing_ ? (self->red_phase_ ? UiTheme::red():dim):UiTheme::text();
        lv_draw_polygon(ctx,&d,p,4);return;
    }
    if(self->layout_==DashboardLayout::ModernMotorsport) {
        const auto center=point(a.x1+220,a.y1+272);arc(ctx,center,210,180,360,dim,14);
        const float thresholds[]={0.f,static_cast<float>(self->yellow_from_),static_cast<float>(self->red_from_),static_cast<float>(self->maximum_)};
        const lv_color_t colors[]={UiTheme::green(),UiTheme::yellow(),UiTheme::red()};const uint32_t ghosts[]={0x122322,0x34321A,0x361D23};
        for(int i=0;i<3;i++) {
            const float lo=std::min(1.f,thresholds[i]/self->maximum_),end=std::min(1.f,thresholds[i+1]/self->maximum_);
            if(end>lo)arc(ctx,center,210,std::lround(180+180*lo),std::lround(180+180*end),lv_color_hex(ghosts[i]),14);
            const float hi=std::min(self->valid_ ? self->fill_/1000.f:0.f,end);
            if(hi>lo)arc(ctx,center,210,std::lround(180+180*lo),std::lround(180+180*hi),colors[i],14);
        }
        if(self->flashing_)arc(ctx,center,210,180,360,self->red_phase_ ? UiTheme::red():dim,14);
        if(self->valid_) { const float angle=180+180*self->fill_/1000.f;line(ctx,polar(center,213,angle),polar(center,191,angle),UiTheme::text(),4); }
        return;
    }
    for(int i=0;i<36;i++) {
        lv_point_t p[4];
        if(self->layout_==DashboardLayout::StripStyle) {
            for(int j=0;j<4;j++)p[j]=point(a.x1+self->strip_blocks_[i][j].x,a.y1+self->strip_blocks_[i][j].y);
        } else {
            const int width=lv_obj_get_width(self->indicator_)-8,x=a.x1+4+width*i/36,end=a.x1+4+width*(i+1)/36-2;
            p[0]=point(x,a.y1);p[1]=point(end,a.y1);p[2]=point(end,a.y1+28);p[3]=point(x,a.y1+28);
        }
        const float lit=self->flashing_ ? (self->red_phase_ ? 1.f:0.f):self->valid_ ? std::clamp(self->fill_*36/1000.f-i,0.f,1.f):0.f;
        const auto color=self->flashing_ ? UiTheme::red():zone(self->maximum_*(i+.5f)/36,self->yellow_from_,self->red_from_);
        lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_mix(color,dim,static_cast<lv_opa_t>(std::lround(lit*255)));lv_draw_polygon(ctx,&d,p,4);
    }
}

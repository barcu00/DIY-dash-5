#include "rpm_scale_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "ui/dashboard_layout.h"
#include "ui/ui_theme.h"
#include "ui/fonts/numeric_fonts.h"
#include "ui/round_geometry.h"

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
bool sport(DashboardLayout layout) {
    return layout==DashboardLayout::AnalogClassic || layout==DashboardLayout::AnalogTwin;
}
// Dial geometry taken from the approved ANALOG CLASSIC / ANALOG TWIN boards.
struct SportDial {
    int cx, cy;                    // centre, relative to the root object
    int disc, disc_ring;           // outer disc and its inner hairline
    int redline;                   // yellow/red zone arc
    int ring, ring_width, segments;
    int trail;                     // thin lit trail inside the segments
    int major_in, major_out, minor_in, minor_out;
    int numbers;                   // radius of the scale numbers
    const lv_font_t* number_font;
    int needle_in, needle_out;
    int face, face_ring;           // centre disc
    int rpm_baseline;              // RPM readout baseline, relative to centre
    int scale_label_y;             // "RPM x1000" baseline, relative to centre
};
const SportDial& sportDial(DashboardLayout layout) {
    static const SportDial classic{214, 218, 214, 206, 194, 170, 22, 54, 155,
        200, 212, 205, 211, 138, &dash_italic_26, 108, 190, 104, 96, -58, 74};
    static const SportDial twin{212, 206, 196, 188, 170, 148, 20, 48, 134,
        175, 186, 180, 186, 116, &dash_italic_22, 96, 164, 86, 78, -44, 126};
    return layout == DashboardLayout::AnalogTwin ? twin : classic;
}
lv_color_t sportColor(TachColor color) {
    // Palette of the approved analog boards.
    switch(color) {
        case TachColor::Yellow: return lv_color_hex(0xF5D000);
        case TachColor::Red: return lv_color_hex(0xFF3B30);
        case TachColor::Off: return lv_color_hex(0x161C22);
        case TachColor::Normal: break;
    }
    return lv_color_hex(0xE4E8EC);
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
    if(sport(layout)) {
        lv_obj_clear_flag(root_,LV_OBJ_FLAG_HIDDEN);
        const bool twin=layout==DashboardLayout::AnalogTwin;
        lv_obj_set_pos(root_,twin ? 8:186,8);lv_obj_set_size(root_,twin ? 432:428,422);
        lv_obj_set_style_border_width(root_,0,0);
        lv_obj_set_pos(indicator_,0,0);lv_obj_set_size(indicator_,twin ? 432:428,422);
        lv_obj_clear_flag(value_,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(caption_,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_font(value_,&dash_italic_22,0);
        lv_obj_set_width(value_,200);lv_label_set_text(value_,"--- RPM");
        const SportDial& dial=sportDial(layout);
        lv_obj_align(value_,LV_ALIGN_TOP_LEFT,dial.cx-100,
                     dial.cy+dial.rpm_baseline-(dash_italic_22.line_height-dash_italic_22.base_line));
        sport_color_=TachColor::Normal;
        lv_obj_set_style_text_color(value_,sportColor(sport_color_),0);
        lv_obj_invalidate(root_);
        return;
    }
    lv_obj_set_style_text_font(value_,&race_digits_96,0);
    if(!dial(layout) && layout!=DashboardLayout::SideGear && layout!=DashboardLayout::StripStyle) {
        lv_obj_add_flag(root_,LV_OBJ_FLAG_HIDDEN);return;
    }
    lv_obj_clear_flag(root_,LV_OBJ_FLAG_HIDDEN);
    const bool side=layout==DashboardLayout::SideGear;
    lv_obj_set_pos(root_,side ? 152:8,side ? 16:8);lv_obj_set_size(root_,dial(layout) ? 448:side ? 640:784,side ? 292:414);
    lv_obj_set_style_border_width(root_,side ? 1:0,0);lv_obj_set_style_border_color(root_,UiTheme::border(),0);lv_obj_set_style_radius(root_,7,0);
    lv_obj_set_pos(indicator_,side ? 16:0,side ? 50:0);lv_obj_set_size(indicator_,dial(layout) ? 448:side ? 608:784,dial(layout) ? 414:side ? 30:150);
    if(layout==DashboardLayout::ModernMotorsport) {
        lv_obj_clear_flag(value_,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(caption_,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(value_,280);lv_label_set_text(value_,"---");const bool e=layout==DashboardLayout::ModernMotorsport;
        lv_obj_align(value_,LV_ALIGN_TOP_MID,-4,e ? 174:166);lv_obj_align(caption_,LV_ALIGN_TOP_MID,-4,e ? 274:263);
        lv_obj_set_style_text_color(value_,UiTheme::text(),0);
    } else { lv_obj_add_flag(value_,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(caption_,LV_OBJ_FLAG_HIDDEN); }
    if(layout==DashboardLayout::StripStyle) {
        // Identical axis-aligned 18x28 rectangles: only their height on the
        // shallow curve changes. Pixel-exact edges, no rotated polygon masks.
        for(int i=0;i<36;i++) {
            const int x=14+21*i;
            const float dx=x+8.5f-392;
            const int y=std::lround(1226-std::sqrt(kStripRadius*kStripRadius-dx*dx));
            strip_blocks_[i]={point(x,y),point(x+17,y),point(x+17,y+27),point(x,y+27)};
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
void RpmScaleView::invalidateModernBand() {
    lv_area_t a;lv_obj_get_coords(indicator_,&a);
    // Include the white cap, but not the entire 448x414 transparent object.
    lv_area_t dirty{static_cast<lv_coord_t>(a.x1+3),static_cast<lv_coord_t>(a.y1+55),
                    static_cast<lv_coord_t>(a.x1+437),static_cast<lv_coord_t>(a.y1+277)};
    lv_obj_invalidate_area(indicator_,&dirty);
}
void RpmScaleView::invalidateAnalogBand() {
    lv_area_t a;lv_obj_get_coords(indicator_,&a);
    // Three clipped bands cover the 270-degree ring without dirtying center digits.
    const int bands[][4]={{8,4,432,124},{8,125,124,365},{316,125,432,365}};
    for(const auto& b:bands) {
        lv_area_t dirty{static_cast<lv_coord_t>(a.x1+b[0]),static_cast<lv_coord_t>(a.y1+b[1]),
                        static_cast<lv_coord_t>(a.x1+b[2]),static_cast<lv_coord_t>(a.y1+b[3])};
        lv_obj_invalidate_area(indicator_,&dirty);
    }
}
void RpmScaleView::invalidateBlocks(uint16_t previous,uint16_t current) {
    lv_area_t a;lv_obj_get_coords(indicator_,&a);
    if(layout_!=DashboardLayout::StripStyle) {
        const int width=lv_obj_get_width(indicator_)-8;
        const int lo=std::clamp<int>(std::min(previous,current)*36/1000,0,35);
        const int hi=std::clamp<int>(std::max(previous,current)*36/1000,0,35);
        a.x2=a.x1+4+width*(hi+1)/36+2;
        a.x1+=4+width*lo/36-2;
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
    if(sport(layout_)) {
        const bool due=!initialized_ || now_ms-last_update_ms_>=25U || raw.valid!=valid_;
        const auto rpm=filter_.sample(raw,now_ms,50U);
        const TachBarState state=tachBarState(rpm.value,rpm.valid,maximum_,shift,now_ms);
        // Needle follows the real value even while the ring flashes full.
        const uint16_t fill=rpmScaleFill(rpm.value,rpm.valid,maximum_);
        if(!initialized_ || rpm.valid!=valid_ || state.color!=sport_color_ || state.flashing!=flashing_) {
            lv_obj_invalidate(indicator_);
            if(state.color!=sport_color_ && state.color!=TachColor::Off)
                lv_obj_set_style_text_color(value_,sportColor(state.color),0);
        } else if(fill!=fill_) invalidateSportSweep(fill_,fill);
        sport_color_=state.color;flashing_=state.flashing;fill_=fill;valid_=rpm.valid;
        if(due) {
            char text[24]="--- RPM";
            if(rpm.valid) {
                const unsigned value=static_cast<unsigned>(std::lround(rpm.value/50.f)*50);
                if(value>=1000)std::snprintf(text,sizeof(text),"%u %03u RPM",value/1000,value%1000);
                else std::snprintf(text,sizeof(text),"%u RPM",value);
            }
            if(std::strcmp(text,last_text_.data())) { lv_label_set_text(value_,text);std::snprintf(last_text_.data(),last_text_.size(),"%s",text); }
            last_update_ms_=now_ms;
        }
        initialized_=true;return;
    }
    const bool flashing=raw.valid && shift.flash_enabled && raw.value>=shift.flash_rpm,red=flashing && now_ms%250U<125U;
    const bool due=!initialized_ || now_ms-last_update_ms_>=25U || raw.valid!=valid_;
    if(!due && flashing==flashing_ && red==red_phase_)return;
    const auto rpm=filter_.sample(raw,now_ms,50U);const auto fill=rpmScaleFill(rpm.value,rpm.valid,maximum_);
    if(!initialized_ || rpm.valid!=valid_ || fill!=fill_ || flashing!=flashing_ || red!=red_phase_) {
        if(layout_==DashboardLayout::AnalogStyle) {
            if(!initialized_ || rpm.valid!=valid_ || flashing!=flashing_ || red!=red_phase_)invalidateAnalogBand();
            invalidateNeedle(fill_);invalidateNeedle(fill);
        }
        else if(!initialized_ || rpm.valid!=valid_ || flashing!=flashing_ || red!=red_phase_) {
            if(layout_==DashboardLayout::ModernMotorsport)invalidateModernBand();
            else lv_obj_invalidate(indicator_);
        }
        else if(layout_==DashboardLayout::ModernMotorsport)invalidateArc(fill_,fill);
        else invalidateBlocks(fill_,fill);
        // Flash the tachometer band only; avoid repainting the large RPM text
        // at each phase transition or every moving-cap sample.
        fill_=fill;valid_=rpm.valid;flashing_=flashing;red_phase_=red;
    }
    if(layout_==DashboardLayout::ModernMotorsport && due) {
        char text[16]="---";if(rpm.valid)std::snprintf(text,sizeof(text),"%.0f",static_cast<double>(rpm.value));
        if(std::strcmp(text,last_text_.data())) { lv_label_set_text(value_,text);std::snprintf(last_text_.data(),last_text_.size(),"%s",text); }
    }
    if(due)last_update_ms_=now_ms;initialized_=true;
}
void RpmScaleView::drawScale(lv_event_t* event) {
    auto* self=static_cast<RpmScaleView*>(lv_event_get_user_data(event));auto* ctx=lv_event_get_draw_ctx(event);
    if(sport(self->layout_)) { self->drawSportScale(ctx);return; }
    lv_area_t a,root;lv_obj_get_coords(self->indicator_,&a);lv_obj_get_coords(self->root_,&root);
    const bool analog=self->layout_==DashboardLayout::AnalogStyle,e=self->layout_==DashboardLayout::ModernMotorsport,curved=self->layout_==DashboardLayout::StripStyle;
    const auto center=point(a.x1+220,a.y1+(e ? 272:212)),strip_center=point(a.x1+392,a.y1+1226);
    const float span=stripSpan(),start=270-span/2;
    if(analog) {
        arc(ctx,center,207,135,405,UiTheme::border(),1);
    } else if(e) {
        arc(ctx,center,219,180,360,UiTheme::border(),1);arc(ctx,center,214,180,360,lv_color_hex(0x183A4B),1);
        line(ctx,point(root.x1+72,root.y1+318),point(root.x1+368,root.y1+318),lv_color_hex(0x183A4B),1);
        line(ctx,point(root.x1+196,root.y1+318),point(root.x1+244,root.y1+318),UiTheme::border(),2);
    } else if(curved) {
        arc(ctx,strip_center,1214,251,289,UiTheme::border(),1);
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
            const auto p=polar(strip_center,kStripRadius+4,angle);
            line(ctx,p,point(p.x,p.y-(major ? 10:5)),zone(rpm,self->yellow_from_,self->red_from_),major ? 2:1);
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
    if(sport(self->layout_)) { self->drawSportIndicator(ctx);return; }
    lv_area_t a;lv_obj_get_coords(self->indicator_,&a);const auto dim=lv_color_hex(0x151D22);
    if(self->layout_==DashboardLayout::AnalogStyle) {
        const auto center=point(a.x1+220,a.y1+212);
        for(int i=0;i<54;i++) {
            const auto color=self->flashing_ ? (self->red_phase_ ? UiTheme::red():dim)
                :zone(self->maximum_*(i+.5f)/54,self->yellow_from_,self->red_from_);
            arc(ctx,center,202,135+i*5,139+i*5,color,10);
        }
        if(!self->valid_)return;
        const float angle=135+270*self->fill_/1000.f,radians=angle*kPi/180.f;
        const auto out=polar(center,205,angle),in=polar(center,175,angle);const float tx=-std::sin(radians)*5,ty=std::cos(radians)*5;
        const lv_point_t p[]={point(std::lround(out.x+tx),std::lround(out.y+ty)),point(std::lround(out.x-tx),std::lround(out.y-ty)),
            point(std::lround(in.x-tx),std::lround(in.y-ty)),point(std::lround(in.x+tx),std::lround(in.y+ty))};
        lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=UiTheme::text();
        lv_draw_polygon(ctx,&d,p,4);return;
    }
    if(self->layout_==DashboardLayout::ModernMotorsport) {
        if(self->flashing_) {
            const auto center=point(a.x1+220,a.y1+272);
            arc(ctx,center,210,180,360,self->red_phase_ ? UiTheme::red():dim,14);
            return;
        }
        const auto center=point(a.x1+220,a.y1+272);arc(ctx,center,210,180,360,dim,14);
        const float thresholds[]={0.f,static_cast<float>(self->yellow_from_),static_cast<float>(self->red_from_),static_cast<float>(self->maximum_)};
        const lv_color_t colors[]={UiTheme::green(),UiTheme::yellow(),UiTheme::red()};const uint32_t ghosts[]={0x122322,0x34321A,0x361D23};
        for(int i=0;i<3;i++) {
            const float lo=std::min(1.f,thresholds[i]/self->maximum_),end=std::min(1.f,thresholds[i+1]/self->maximum_);
            if(end>lo)arc(ctx,center,210,std::lround(180+180*lo),std::lround(180+180*end),lv_color_hex(ghosts[i]),14);
            const float hi=std::min(self->valid_ ? self->fill_/1000.f:0.f,end);
            if(hi>lo)arc(ctx,center,210,std::lround(180+180*lo),std::lround(180+180*hi),colors[i],14);
        }
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
        lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_mix(color,dim,static_cast<lv_opa_t>(std::lround(lit*255)));
        const lv_area_t block{p[0].x,p[0].y,p[2].x,p[2].y};lv_draw_rect(ctx,&d,&block);
    }
}

lv_point_t RpmScaleView::sportCenter() const {
    lv_area_t a;lv_obj_get_coords(root_,&a);
    const SportDial& dial=sportDial(layout_);
    return point(a.x1+dial.cx,a.y1+dial.cy);
}
void RpmScaleView::invalidateSportSweep(uint16_t previous,uint16_t current) {
    const auto c=sportCenter();const SportDial& dial=sportDial(layout_);
    const float lo=135+270*std::min(previous,current)/1000.f,hi=135+270*std::max(previous,current)/1000.f;
    lv_area_t dirty=RoundGeometry::sectorBounds(c,dial.needle_in-6,dial.needle_out+8,lo-2,hi+2);
    dirty.x1-=10;dirty.y1-=10;dirty.x2+=10;dirty.y2+=10;
    lv_obj_invalidate_area(indicator_,&dirty);
}
namespace {
// Radial gradient approximated by a few concentric discs (round on the panel).
void radialDisc(lv_draw_ctx_t* ctx,lv_point_t c,int r,lv_color_t outer,lv_color_t inner,int steps) {
    for(int i=0;i<steps;i++) {
        const int radius=r-(r*i)/(steps+1);
        RoundGeometry::disc(ctx,c,radius,lv_color_mix(inner,outer,static_cast<lv_opa_t>(255*i/steps)));
    }
}
// Ring of outer radius r and width w (same convention as lv_draw_arc).
void ring(lv_draw_ctx_t* ctx,lv_point_t c,int r,float start,float end,lv_color_t color,int w,lv_opa_t opa=LV_OPA_COVER) {
    RoundGeometry::arc(ctx,c,r-w/2.f,static_cast<float>(w),start,end,color,opa);
}
void spacedText(lv_draw_ctx_t* ctx,int cx,int baseline,const char* text,const lv_font_t* font,lv_color_t color,int space) {
    lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=font;d.color=color;d.letter_space=static_cast<lv_coord_t>(space);d.align=LV_TEXT_ALIGN_CENTER;
    const int top=baseline-(font->line_height-font->base_line);
    lv_area_t a{static_cast<lv_coord_t>(cx-90),static_cast<lv_coord_t>(top),static_cast<lv_coord_t>(cx+90),static_cast<lv_coord_t>(top+font->line_height)};
    lv_draw_label(ctx,&d,&a,text,nullptr);
}
void polygon(lv_draw_ctx_t* ctx,const lv_point_t* p,int n,lv_color_t color,lv_opa_t opa=LV_OPA_COVER) {
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=color;d.bg_opa=opa;lv_draw_polygon(ctx,&d,p,static_cast<uint16_t>(n));
}
}  // namespace
void RpmScaleView::drawSportScale(lv_draw_ctx_t* ctx) {
    const auto c=sportCenter();const SportDial& dial=sportDial(layout_);
    const bool classic=layout_==DashboardLayout::AnalogClassic;
    // Disc with a soft centre glow, hairline rims.
    radialDisc(ctx,c,dial.disc,lv_color_hex(0x000203),lv_color_hex(0x0E151C),6);
    ring(ctx,c,dial.disc,0,360,lv_color_hex(0x262D35),2);
    ring(ctx,c,dial.disc_ring,0,360,lv_color_hex(0x11171D),1);
    const float max=static_cast<float>(maximum_);
    const auto angleFor=[&](float rpm){ return 135+270*std::clamp(rpm/max,0.f,1.f); };
    if(yellow_from_<maximum_) ring(ctx,c,dial.redline,std::lround(angleFor(yellow_from_)),std::lround(angleFor(std::min(red_from_,maximum_))),lv_color_hex(0xF5D000),4);
    if(red_from_<maximum_) ring(ctx,c,dial.redline,std::lround(angleFor(red_from_)),405,lv_color_hex(0xFF3B30),4);
    for(int rpm=0;rpm<=maximum_;rpm+=250) {
        const bool major=rpm%1000==0;const float angle=angleFor(rpm);
        line(ctx,RoundGeometry::point(c,major ? dial.major_in:dial.minor_in,angle),RoundGeometry::point(c,major ? dial.major_out:dial.minor_out,angle),
             major ? lv_color_hex(0xE6EAEE):lv_color_hex(0x4A525B),major ? 3:1);
        if(major) {
            char text[4];std::snprintf(text,sizeof(text),"%d",rpm/1000);
            const auto p=RoundGeometry::point(c,dial.numbers,angle);
            const lv_font_t* f=dial.number_font;
            lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=f;d.align=LV_TEXT_ALIGN_CENTER;
            d.color=rpm>=red_from_ ? lv_color_hex(0xFF6A60):lv_color_hex(0xC9CED4);
            const lv_area_t box{static_cast<lv_coord_t>(p.x-20),static_cast<lv_coord_t>(p.y-f->line_height/2),
                                static_cast<lv_coord_t>(p.x+20),static_cast<lv_coord_t>(p.y+f->line_height/2)};
            lv_draw_label(ctx,&d,&box,text,nullptr);
        }
    }
    // Centre face.
    radialDisc(ctx,c,dial.face,lv_color_hex(0x030507),lv_color_hex(0x121A22),4);
    ring(ctx,c,dial.face,0,360,lv_color_hex(0x2E363F),2);
    ring(ctx,c,dial.face_ring,0,360,lv_color_hex(0x141B22),1);
    spacedText(ctx,c.x,c.y+dial.scale_label_y,"RPM \xC3\x97" "1000",&dash_label_12,lv_color_hex(0x5E6872),2);
    lv_area_t r;lv_obj_get_coords(root_,&r);
    if(classic) {
        // Gear hexagon and the base lines either side of it.
        const float k=RoundGeometry::scaleX();
        const auto hx=[&](int dx){ return static_cast<int>(std::lround(c.x+dx*k)); };
        const lv_point_t hex[6]={point(hx(-42),c.y+152),point(hx(-21),c.y+117),point(hx(21),c.y+117),
                                 point(hx(42),c.y+152),point(hx(21),c.y+187),point(hx(-21),c.y+187)};
        polygon(ctx,hex,6,lv_color_hex(0x070A0E));
        for(int i=0;i<6;i++)line(ctx,hex[i],hex[(i+1)%6],lv_color_hex(0xC9CED4),2);
        const int y=c.y+192;
        line(ctx,point(hx(-186),y),point(hx(-70),y),lv_color_hex(0x262D35),2);
        line(ctx,point(hx(-70),y),point(hx(-60),y-10),lv_color_hex(0x262D35),2);
        line(ctx,point(hx(186),y),point(hx(70),y),lv_color_hex(0x262D35),2);
        line(ctx,point(hx(70),y),point(hx(60),y-10),lv_color_hex(0x262D35),2);
    } else {
        const int y=c.y+198;const auto col=lv_color_hex(0x262D35);
        line(ctx,point(c.x-70,y),point(c.x-20,y),col,2);line(ctx,point(c.x-20,y),point(c.x-10,y-10),col,2);
        line(ctx,point(c.x-10,y-10),point(c.x+10,y-10),col,2);line(ctx,point(c.x+10,y-10),point(c.x+20,y),col,2);
        line(ctx,point(c.x+20,y),point(c.x+70,y),col,2);
    }
}
void RpmScaleView::drawSportIndicator(lv_draw_ctx_t* ctx) {
    const auto c=sportCenter();const SportDial& dial=sportDial(layout_);
    const auto unlit=lv_color_hex(0x1A2129);
    const auto lit_color=sportColor(sport_color_);
    const float step=270.f/dial.segments,on=step*0.72f;
    for(int i=0;i<dial.segments;i++) {
        const float at=(i+.5f)*1000.f/dial.segments;
        const bool lit=flashing_ ? sport_color_!=TachColor::Off : valid_ && at<=fill_;
        const float start=135+i*step;
        ring(ctx,c,dial.ring,std::lround(start),std::lround(start+on),lit ? lit_color:unlit,dial.ring_width);
    }
    if(!valid_)return;
    const float angle=135+270*fill_/1000.f;
    if(!flashing_) {
        ring(ctx,c,dial.trail,135,angle,lit_color,3,140);
    }
    // Needle: glow wedge in the stage colour under a slim white wedge.
    const float rad=angle*kPi/180.f,nx=-std::sin(rad)*RoundGeometry::scaleX(),ny=std::cos(rad);
    const auto base=RoundGeometry::point(c,dial.needle_in,angle),tip=RoundGeometry::point(c,dial.needle_out,angle);
    const auto at=[&](lv_point_t p,float w){ return point(std::lround(p.x+nx*w),std::lround(p.y+ny*w)); };
    const lv_point_t glow[4]={at(base,9),at(tip,2.5f),at(tip,-2.5f),at(base,-9)};
    polygon(ctx,glow,4,sport_color_==TachColor::Normal ? lv_color_hex(0x8E9AA6):lit_color,90);
    const lv_point_t needle[4]={at(base,4.5f),at(tip,1),at(tip,-1),at(base,-4.5f)};
    polygon(ctx,needle,4,lv_color_hex(0xFFFFFF));
}

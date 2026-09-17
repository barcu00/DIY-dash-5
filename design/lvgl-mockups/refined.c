/* Approval-only LVGL prototypes. Does not alter any production firmware view.
 * Reuse the frozen approved helper/font/navigation set without changing it. */
#define main frozen_approval_main
#include "mockup.c"
#undef main
#include <assert.h>

static float maximum_rpm=10000.f;
static const float yellow_from=5500.f, red_from=7000.f, current_rpm=6840.f;
static unsigned rpm_zone(float rpm) {
    return rpm>=red_from ? RED : rpm>=yellow_from ? YELLOW : GREEN;
}
static void true_arc(lv_draw_ctx_t *ctx,int cx,int cy,int radius,
                     int start,int end,unsigned rgb,int width) {
    lv_draw_arc_dsc_t d; lv_draw_arc_dsc_init(&d);
    d.color=color(rgb); d.width=width; d.rounded=0;
    lv_point_t center={cx,cy}; lv_draw_arc(ctx,&d,&center,radius,start,end);
}
static lv_point_t circle_point(int cx,int cy,float radius,float angle) {
    float a=angle*3.14159265f/180.f;
    return (lv_point_t){lroundf(cx+radius*cosf(a)),lroundf(cy+radius*sinf(a))};
}
static void analog_motorsport(lv_draw_ctx_t *ctx) {
    const int cx=228,cy=220;
    true_arc(ctx,cx,cy,207,135,405,FRAME,1);
    // Exactly 54 congruent circular segments: 4 degrees ON + 1 degree gap.
    // Native LVGL arcs replace polygonal approximations of the ring.
    for(int i=0;i<54;i++)
        true_arc(ctx,cx,cy,202,135+i*5,139+i*5,
                 rpm_zone(maximum_rpm*(i+.5f)/54.f),10);
    for(int rpm=0;rpm<=maximum_rpm;rpm+=200) {
        float f=rpm/maximum_rpm;
        lv_point_t a=polar(cx,cy,187,f),b=polar(cx,cy,rpm%1000 ? 181:174,f);
        line(ctx,a.x,a.y,b.x,b.y,rpm%1000 ? MUTED:WHITE,rpm%1000 ? 1:2);
        if(rpm%1000==0) {
            lv_point_t p=polar(cx,cy,153,f);
            if(rpm==0)p.x-=24; if(rpm==maximum_rpm)p.x+=24;
            char s[8]; snprintf(s,sizeof(s),"%d",rpm/1000);
            text(ctx,p.x-30,p.y-13,60,s,&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
        }
    }
    text(ctx,148,101,160,"RPM x1000",&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_CENTER);
    const lv_point_t tip=polar(cx,cy,174,current_rpm/maximum_rpm);
    const float dx=tip.x-cx,dy=tip.y-cy,length=hypotf(dx,dy);
    // Broad white body with a prominent red spearhead.
    lv_point_t needle[3]={{lroundf(cx-dy*7.f/length),lroundf(cy+dx*7.f/length)},tip,
                          {lroundf(cx+dy*7.f/length),lroundf(cy-dx*7.f/length)}};
    polygon(ctx,needle,3,WHITE);
    lv_point_t tail=polar(cx,cy,112,current_rpm/maximum_rpm);
    lv_point_t head[3]={{lroundf(tail.x-dy*5.f/length),lroundf(tail.y+dx*5.f/length)},tip,
                       {lroundf(tail.x+dy*5.f/length),lroundf(tail.y-dx*5.f/length)}};
    polygon(ctx,head,3,RED);
    rect(ctx,cx-10,cy-10,21,21,BLACK,WHITE,11);
    text(ctx,88,305,280,"6840",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,128,386,200,"RPM",&lv_font_montserrat_20,MUTED,LV_TEXT_ALIGN_CENTER);
    const char *titles[]={"SPEED","OIL PRESS","OIL TEMP","CLT","MAP","LAMBDA"};
    const char *values[]={"137","4.8","108","91","1.18","0.86"};
    const char *units[]={"km/h","bar","°C","°C","bar",""};
    for(int i=0;i<6;i++) {
        int y=16+i*68; rect(ctx,464,y,328,62,BLACK,FRAME,6);
        text(ctx,476,y+22,120,titles[i],&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_CENTER);
        lv_point_t value_size,unit_size;
        lv_txt_get_size(&value_size,values[i],&race_digits_48,0,0,400,LV_TEXT_FLAG_NONE);
        lv_txt_get_size(&unit_size,units[i],&lv_font_montserrat_16,0,0,400,LV_TEXT_FLAG_NONE);
        int gap=units[i][0] ? 8:0;
        int x=692-(value_size.x+gap+unit_size.x)/2;
        text(ctx,x,y+7,value_size.x+1,values[i],&race_digits_48,WHITE,LV_TEXT_ALIGN_LEFT);
        text(ctx,x+value_size.x+gap,y+29,unit_size.x+1,units[i],&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_LEFT);
        if(i==2 || i==3)slim_temperature(ctx,486,y+55,294,i==2 ? .75f:.57f);
    }
}
static void circular_strip(lv_draw_ctx_t *ctx) {
    const int cx=400,cy=1234;
    const float radius=1196.f;
    const float span=2*asinf(379.f/radius)*180.f/3.14159265f;
    const float start=270-span/2, step=span/36;
    assert(span>36 && span<38);
    true_arc(ctx,cx,cy,1214,251,289,CYAN,1);
    // Exact annular sectors, NOT slanted polygon blocks. Supersampling avoids
    // integer-degree arc quantisation across this shallow 37-degree sweep.
    // Approval-only rasterisation through LVGL primitives, not firmware code.
    for(int y=36;y<132;y++) for(int x=8;x<792;x++) {
        unsigned sum_r=0,sum_g=0,sum_b=0; int hits=0;
        for(int sy=0;sy<4;sy++) for(int sx=0;sx<4;sx++) {
            float dx=x+(sx+.5f)/4-cx,dy=y+(sy+.5f)/4-cy;
            float r=hypotf(dx,dy),angle=atan2f(dy,dx)*180.f/3.14159265f+360;
            float index=(angle-start)/step; int i=(int)floorf(index);
            if(r<radius-28 || r>radius || i<0 || i>=36 ||
               index-i<.10f || index-i>.90f)continue;
            float lit=fmaxf(0,fminf(1,current_rpm/maximum_rpm*36-i));
            lv_color32_t c; c.full=lv_color_to32(lv_color_mix(
                color(rpm_zone(maximum_rpm*(i+.5f)/36)),color(DIM),lroundf(lit*255)));
            sum_r+=c.ch.red;sum_g+=c.ch.green;sum_b+=c.ch.blue;hits++;
        }
        if(hits)rect(ctx,x,y,1,1,((sum_r/16)<<16)|((sum_g/16)<<8)|(sum_b/16),0,0);
    }
    for(int rpm=0;rpm<=maximum_rpm;rpm+=200) {
        const float f=rpm/maximum_rpm,angle=start+span*f;
        lv_point_t a=circle_point(cx,cy,radius+4,angle),b=circle_point(cx,cy,radius+(rpm%1000 ? 9:14),angle);
        line(ctx,a.x,a.y,b.x,b.y,rpm_zone(rpm),rpm%1000 ? 1:2);
        if(rpm%1000==0) {
            lv_point_t p=circle_point(cx,cy,radius-49,angle);
            char s[8]; snprintf(s,sizeof(s),"%d",rpm/1000);
            text(ctx,p.x-24,p.y-8,48,s,&lv_font_montserrat_16,WHITE,LV_TEXT_ALIGN_CENTER);
        }
    }
    text(ctx,650,6,140,"RPM x1000",&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_RIGHT);
    small(ctx,8,178,158,76,"OIL PRESS","4.8","bar",CYAN,0);
    small(ctx,8,262,158,76,"OIL TEMP","108","°C",ORANGE,1);
    small(ctx,8,346,158,76,"FUEL PRESS","4.1","bar",GREEN,0);
    small(ctx,634,178,158,76,"CLT","91","°C",GREEN,2);
    small(ctx,634,262,158,76,"IAT","32","°C",CYAN,0);
    small(ctx,634,346,158,76,"LAMBDA","0.86","",YELLOW,0);
    line(ctx,400,218,400,404,FRAME,1);
    text(ctx,176,238,218,"6840",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,410,238,214,"137",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,176,350,218,"RPM",&lv_font_montserrat_24,MUTED,LV_TEXT_ALIGN_CENTER);
    text(ctx,410,350,214,"km/h",&lv_font_montserrat_24,MUTED,LV_TEXT_ALIGN_CENTER);
}
static void draw_refined(lv_event_t *event) {
    lv_draw_ctx_t *ctx=lv_event_get_draw_ctx(event);
    if(preset==0 || preset==5)analog_motorsport(ctx);
    else if(preset==1 || preset==6)circular_strip(ctx);
    else if(preset==2)side(ctx);
    else classic(ctx,preset==4);
    nav(ctx);
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    lv_init(); lv_disp_draw_buf_t buffer;
    lv_disp_draw_buf_init(&buffer,draw_buffer,NULL,800*40);
    lv_disp_drv_t driver; lv_disp_drv_init(&driver);
    driver.hor_res=800; driver.ver_res=480; driver.draw_buf=&buffer;
    driver.flush_cb=flush; lv_disp_drv_register(&driver);
    lv_obj_t *screen=lv_scr_act(); lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen,color(BLACK),0);
    lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen,draw_refined,LV_EVENT_DRAW_MAIN,NULL);
    const char *names[]={"lvgl-analog-motorsport-v3","lvgl-strip-circular-v3",
        "lvgl-side-gear-v3","lvgl-classic-dash-v3","lvgl-classic-track-v3",
        "lvgl-analog-7500-v3","lvgl-strip-7500-v3"};
    for(preset=0;preset<7;preset++) {
        maximum_rpm=preset>=5 ? 7500.f:10000.f;
        memset(framebuffer,0,sizeof(framebuffer));
        lv_obj_invalidate(screen); lv_refr_now(NULL);
        char path[512]; snprintf(path,sizeof(path),"%s/%s.ppm",argv[1],names[preset]);
        FILE *out=fopen(path,"wb"); if(!out)return 3;
        fprintf(out,"P6\n800 480\n255\n");
        assert(fwrite(framebuffer,1,sizeof(framebuffer),out)==sizeof(framebuffer));
        fclose(out);
    }
    return 0;
}

#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(race_digits_48);
LV_FONT_DECLARE(race_digits_64);
LV_FONT_DECLARE(race_digits_96);
LV_FONT_DECLARE(race_digits_140);

enum { BLACK=0x000203, FRAME=0x45677C, WHITE=0xF8FBFF, MUTED=0xA7BED4,
       CYAN=0x00D5F4, GREEN=0x00EB58, YELLOW=0xEEEF00, ORANGE=0xFF9200,
       RED=0xFF2020, DIM=0x151D22 };
static unsigned char framebuffer[480][800][3];
static lv_color_t draw_buffer[800 * 40];
static int preset;
static lv_color_t color(unsigned rgb) { return lv_color_hex(rgb); }

static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels) {
    for(int y=area->y1;y<=area->y2;y++) for(int x=area->x1;x<=area->x2;x++) {
        lv_color32_t pixel; pixel.full=lv_color_to32(*pixels++);
        if(x>=0 && x<800 && y>=0 && y<480) {
            framebuffer[y][x][0]=pixel.ch.red;
            framebuffer[y][x][1]=pixel.ch.green;
            framebuffer[y][x][2]=pixel.ch.blue;
        }
    }
    lv_disp_flush_ready(driver);
}
static void rect(lv_draw_ctx_t *ctx,int x,int y,int w,int h,unsigned fill,unsigned border,int radius) {
    lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
    d.bg_color=color(fill); d.radius=radius;
    d.border_width=border ? 1 : 0; d.border_color=color(border);
    lv_area_t a={x,y,x+w-1,y+h-1}; lv_draw_rect(ctx,&d,&a);
}
static void line(lv_draw_ctx_t *ctx,int x1,int y1,int x2,int y2,unsigned rgb,int width) {
    lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d);
    d.color=color(rgb); d.width=width; d.round_start=d.round_end=1;
    lv_point_t a={x1,y1},b={x2,y2}; lv_draw_line(ctx,&d,&a,&b);
}
static void text(lv_draw_ctx_t *ctx,int x,int y,int w,const char *s,const lv_font_t *f,unsigned rgb,lv_text_align_t align) {
    lv_draw_label_dsc_t d; lv_draw_label_dsc_init(&d);
    d.color=color(rgb); d.font=f; d.align=align;
    lv_area_t a={x,y,x+w-1,y+f->line_height+2}; lv_draw_label(ctx,&d,&a,s,NULL);
}
static lv_point_t polar(int cx,int cy,float r,float f) {
    float angle=(135+270*f)*3.14159265f/180;
    lv_point_t p={lroundf(cx+r*cosf(angle)),lroundf(cy+r*sinf(angle))}; return p;
}
static void polygon(lv_draw_ctx_t *ctx,lv_point_t *p,int n,unsigned rgb) {
    lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d); d.bg_color=color(rgb);
    lv_draw_polygon(ctx,&d,p,n);
}
static void arc_line(lv_draw_ctx_t *ctx,int cx,int cy,int r,unsigned rgb,int width) {
    for(int i=0;i<100;i++) { lv_point_t a=polar(cx,cy,r,i/100.f),b=polar(cx,cy,r,(i+1)/100.f);
        line(ctx,a.x,a.y,b.x,b.y,rgb,width); }
}
static void nav(lv_draw_ctx_t *ctx) {
    const int track=preset==4;
    const unsigned dash_color=track ? MUTED:CYAN;
    const unsigned track_color=track ? CYAN:MUTED;
    line(ctx,8,434,792,434,FRAME,1);
    line(ctx,266,444,266,474,FRAME,1); line(ctx,533,444,533,474,FRAME,1);
    arc_line(ctx,82,459,13,dash_color,2); line(ctx,82,459,88,450,dash_color,2);
    rect(ctx,80,457,5,5,dash_color,0,3);
    text(ctx,109,447,130,"DASH",&lv_font_montserrat_20,dash_color,LV_TEXT_ALIGN_LEFT);
    for(int r=0;r<3;r++) for(int c=0;c<4;c++) if((r+c)%2==0)
        rect(ctx,350+c*5+r,447+r*5,5,5,track_color,0,0);
    line(ctx,351,446,346,472,track_color,2);
    text(ctx,384,447,120,"TRACK",&lv_font_montserrat_20,track_color,LV_TEXT_ALIGN_LEFT);
    for(int i=0;i<8;i++) { float a=i*3.14159265f/4;
        line(ctx,623+9*cosf(a),459+9*sinf(a),623+14*cosf(a),459+14*sinf(a),MUTED,4); }
    rect(ctx,613,449,21,21,BLACK,MUTED,11); rect(ctx,619,455,9,9,BLACK,MUTED,5);
    text(ctx,651,447,142,"SETTINGS",&lv_font_montserrat_20,MUTED,LV_TEXT_ALIGN_LEFT);
    line(ctx,track ? 319:52,478,track ? 487:220,478,CYAN,3);
}
static void slim_temperature(lv_draw_ctx_t *ctx,int x,int y,int width,float f) {
    rect(ctx,x,y,width,3,DIM,0,1); rect(ctx,x,y,lroundf(width*f),3,GREEN,0,1);
}
static void small(lv_draw_ctx_t *ctx,int x,int y,int w,int h,const char *title,const char *value,const char *unit,unsigned rail,int temp) {
    rect(ctx,x,y,w,h,BLACK,FRAME,6);
    if(h<90) {
        text(ctx,x+15,y+7,w-25,title,&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_LEFT);
        text(ctx,x+12,y+26,unit[0] ? 96 : w-24,value,&race_digits_48,WHITE,LV_TEXT_ALIGN_CENTER);
        text(ctx,x+108,y+43,w-114,unit,&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_LEFT);
    } else {
        text(ctx,x+12,y+9,w-24,title,&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_CENTER);
        text(ctx,x+12,y+31,w-24,value,&race_digits_48,WHITE,LV_TEXT_ALIGN_CENTER);
        text(ctx,x+12,y+h-27,w-24,unit,&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_CENTER);
    }
    if(temp) slim_temperature(ctx,x+14,y+h-7,w-28,temp==1 ?.75f:.57f);
}
static void rpm_bar(lv_draw_ctx_t *ctx,int x,int y,int width,int curved) {
    const int count=36;
    float progress=.684f;
    for(int i=0;i<count;i++) {
        float a=i/(float)count,b=(i+1)/(float)count;
        int x1=x+width*a,x2=x+width*b-2;
        int y1=y+(curved ? 62*(2*a-1)*(2*a-1):0);
        int y2=y+(curved ? 62*(2*b-1)*(2*b-1):0);
        int slant=curved ? 5:0;
        lv_point_t p[4]={{x1+slant,y1},{x2+slant,y2},{x2,y2+28},{x1,y1+28}};
        unsigned rgb=i<12 ? GREEN : i<24 ? YELLOW : RED;
        polygon(ctx,p,4,DIM);
        float lit=fmaxf(0,fminf(1,(progress-a)/(b-a)));
        if(lit>0) { p[1].x=p[0].x+(p[1].x-p[0].x)*lit; p[1].y=p[0].y+(p[1].y-p[0].y)*lit;
            p[2].x=p[3].x+(p[2].x-p[3].x)*lit; p[2].y=p[3].y+(p[2].y-p[3].y)*lit;
            polygon(ctx,p,4,rgb); }
    }
    for(int i=0;i<=50;i++) {
        float f=i/50.f; int px=x+width*f,py=y+(curved ? 62*(2*f-1)*(2*f-1):0);
        unsigned rgb=f>=.8f ? RED : WHITE;
        line(ctx,px,py-10,px,py-4,rgb,i%5==0 ? 2:1);
        if(i%5==0) { char s[8]; snprintf(s,sizeof(s),"%d",i/5);
            line(ctx,px,py+33,px,py+38,rgb,1);
            int tx=px-14; if(tx<x-4)tx=x-4; if(tx>x+width-24)tx=x+width-24;
            text(ctx,tx,py+43,28,s,&lv_font_montserrat_16,WHITE,LV_TEXT_ALIGN_CENTER); }
    }
    if(curved) for(int i=0;i<80;i++) {
        float a=i/80.f,b=(i+1)/80.f;
        line(ctx,x+width*a,y+62*(2*a-1)*(2*a-1)-17,x+width*b,y+62*(2*b-1)*(2*b-1)-17,CYAN,1);
    }
}
static void analog(lv_draw_ctx_t *ctx) {
    const int cx=228,cy=222;
    arc_line(ctx,cx,cy,207,FRAME,1);
    arc_line(ctx,cx,cy,111,0x142B38,1);
    for(int i=0;i<100;i++) {
        float a=i/100.f,b=(i+.72f)/100.f;
        lv_point_t p[4]={polar(cx,cy,194,a),polar(cx,cy,202,a),polar(cx,cy,202,b),polar(cx,cy,194,b)};
        unsigned rgb=i<20 ? CYAN : i<66 ? GREEN : i<80 ? YELLOW : RED;
        polygon(ctx,p,4,rgb);
        lv_point_t out=polar(cx,cy,190,a),in=polar(cx,cy,i%10==0 ? 176:184,a);
        line(ctx,out.x,out.y,in.x,in.y,WHITE,i%10==0 ? 3:1);
        if(i%10==0) { lv_point_t t=polar(cx,cy,159,a); char s[8]; snprintf(s,sizeof(s),"%d",i/10);
            text(ctx,t.x-18-(i==0 ? 20:0),t.y-12,36,s,&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER); }
    }
    lv_point_t ten=polar(cx,cy,159,1); text(ctx,ten.x,ten.y-12,40,"10",&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
    lv_point_t tip=polar(cx,cy,168,.684f);
    float dx=tip.x-cx,dy=tip.y-cy,len=sqrtf(dx*dx+dy*dy);
    lv_point_t needle[3]={{cx-dy*4/len,cy+dx*4/len},{tip.x,tip.y},{cx+dy*4/len,cy-dx*4/len}};
    polygon(ctx,needle,3,RED); rect(ctx,cx-9,cy-9,19,19,DIM,FRAME,10);
    text(ctx,88,305,280,"6840",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,128,386,200,"RPM",&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
    const char *titles[]={"SPEED","OIL P","OIL T","CLT","BOOST","LAMBDA"};
    const char *values[]={"137","4.8","108","91","1.18","0.86"};
    const char *units[]={"km/h","bar","°C","°C","bar",""};
    unsigned rails[]={CYAN,ORANGE,ORANGE,CYAN,GREEN,YELLOW};
    for(int i=0;i<6;i++) {
        int y=16+i*68; rect(ctx,464,y,328,62,BLACK,FRAME,6);
        text(ctx,486,y+22,115,titles[i],&lv_font_montserrat_16,WHITE,LV_TEXT_ALIGN_LEFT);
        text(ctx,592,y+2,123,values[i],&race_digits_48,WHITE,LV_TEXT_ALIGN_RIGHT);
        text(ctx,724,y+31,62,units[i],&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_LEFT);
        if(i==2 || i==3)slim_temperature(ctx,486,y+55,294,i==2 ?.75f:.57f);
    }
}
static void side(lv_draw_ctx_t *ctx) {
    rect(ctx,8,16,136,292,BLACK,FRAME,7); rect(ctx,152,16,640,292,BLACK,FRAME,7);
    text(ctx,18,42,116,"GEAR",&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,18,116,116,"3",&race_digits_140,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,172,28,240,"RPM x1000",&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_LEFT);
    rpm_bar(ctx,172,66,600,0);
    text(ctx,170,155,292,"6840",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,480,155,292,"137",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,170,260,292,"RPM",&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,480,260,292,"km/h",&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
    small(ctx,8,316,150,106,"OIL P","4.8","bar",CYAN,0);
    small(ctx,166,316,150,106,"OIL T","108","°C",ORANGE,1);
    small(ctx,324,316,150,106,"CLT","91","°C",GREEN,2);
    small(ctx,482,316,150,106,"BOOST","1.18","bar",CYAN,0);
    small(ctx,640,316,152,106,"LAMBDA","0.86","",YELLOW,0);
}
static void strip(lv_draw_ctx_t *ctx) {
    text(ctx,654,6,136,"RPM x1000",&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_RIGHT);
    rpm_bar(ctx,18,38,758,1);
    small(ctx,8,178,158,76,"OIL P","4.8","bar",CYAN,0);
    small(ctx,8,262,158,76,"OIL T","108","°C",ORANGE,1);
    small(ctx,8,346,158,76,"FUEL P","4.1","bar",GREEN,0);
    small(ctx,634,178,158,76,"CLT","91","°C",RED,2);
    small(ctx,634,262,158,76,"IAT","32","°C",CYAN,0);
    small(ctx,634,346,158,76,"LAMBDA","0.86","",GREEN,0);
    line(ctx,400,218,400,404,FRAME,1);
    text(ctx,176,238,218,"6840",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,410,238,214,"137",&race_digits_96,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,176,350,218,"RPM",&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,410,350,214,"km/h",&lv_font_montserrat_24,WHITE,LV_TEXT_ALIGN_CENTER);
}
static void wide(lv_draw_ctx_t *ctx,int y,const char *title,const char *value,const char *unit,unsigned rail) {
    rect(ctx,192,y,416,90,BLACK,FRAME,6);
    text(ctx,204,y+7,392,title,&lv_font_montserrat_16,MUTED,LV_TEXT_ALIGN_CENTER);
    text(ctx,232,y+29,336,value,&race_digits_64,WHITE,LV_TEXT_ALIGN_CENTER);
    text(ctx,548,y+56,52,unit,&lv_font_montserrat_12,MUTED,LV_TEXT_ALIGN_RIGHT);
}
static void classic(lv_draw_ctx_t *ctx,int track) {
    // Original 14-slot DASH / 12-slot TRACK geometry is intentionally retained.
    const int rows[]={36,134,232,332};
    for(int i=0;i<12;i++) {
        unsigned rgb=i<4 ? GREEN : i<8 ? YELLOW : RED;
        rect(ctx,8+i*66,8,60,16,rgb,0,3);
    }
    small(ctx,8,rows[0],176,90,"SPEED","137","km/h",CYAN,0);
    small(ctx,8,rows[1],176,90,track ? "CLT":"LAMBDA",track ? "91":"0.86",track ? "°C":"",track ? GREEN:YELLOW,track ? 2:0);
    small(ctx,8,rows[2],176,90,track ? "OIL T":"CLT",track ? "108":"91","°C",track ? ORANGE:GREEN,track ? 1:2);
    small(ctx,8,rows[3],176,90,track ? "BATTERY":"OIL P",track ? "14.2":"4.8",track ? "V":"bar",CYAN,0);
    small(ctx,616,rows[0],176,90,track ? "OIL P":"TPS",track ? "4.8":"78",track ? "bar":"%",track ? ORANGE:CYAN,0);
    small(ctx,616,rows[1],176,90,track ? "LAMBDA":"IAT",track ? "0.86":"32",track ? "":"°C",track ? YELLOW:CYAN,0);
    small(ctx,616,rows[2],176,90,track ? "IAT":"OIL T",track ? "32":"108","°C",track ? CYAN:ORANGE,track ? 0:1);
    small(ctx,616,rows[3],176,90,track ? "TPS":"BATTERY",track ? "78":"14.2",track ? "%":"V",CYAN,0);
    wide(ctx,rows[0],"RPM","6840","rpm",CYAN);
    wide(ctx,rows[1],"GEAR","3","",CYAN);
    if(track) {
        wide(ctx,rows[2],"BOOST","1.18","bar",CYAN);
        wide(ctx,rows[3],"FUEL P","4.1","bar",GREEN);
    } else {
        small(ctx,192,rows[2],204,90,"BOOST","1.18","bar",CYAN,0);
        small(ctx,404,rows[2],204,90,"FUEL P","4.1","bar",GREEN,0);
        small(ctx,192,rows[3],204,90,"TPS","78","%",CYAN,0);
        small(ctx,404,rows[3],204,90,"BATTERY","14.2","V",CYAN,0);
    }
}
static void draw(lv_event_t *event) {
    lv_draw_ctx_t *ctx=lv_event_get_draw_ctx(event);
    if(preset==0)analog(ctx); else if(preset==1)side(ctx); else if(preset==2)strip(ctx);
    else classic(ctx,preset==4);
    nav(ctx);
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    lv_init(); lv_disp_draw_buf_t buffer; lv_disp_draw_buf_init(&buffer,draw_buffer,NULL,800*40);
    lv_disp_drv_t driver; lv_disp_drv_init(&driver); driver.hor_res=800; driver.ver_res=480;
    driver.draw_buf=&buffer; driver.flush_cb=flush; lv_disp_drv_register(&driver);
    lv_obj_t *screen=lv_scr_act(); lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen,color(BLACK),0); lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE); lv_obj_add_event_cb(screen,draw,LV_EVENT_DRAW_MAIN,NULL);
    const char *names[]={"lvgl-analog-style","lvgl-side-gear","lvgl-strip-style","lvgl-classic-dash","lvgl-classic-track"};
    for(preset=0;preset<5;preset++) {
        memset(framebuffer,0,sizeof(framebuffer)); lv_obj_invalidate(screen); lv_refr_now(NULL);
        char path[512]; snprintf(path,sizeof(path),"%s/%s.ppm",argv[1],names[preset]);
        FILE *out=fopen(path,"wb"); if(!out)return 3;
        fprintf(out,"P6\n800 480\n255\n"); fwrite(framebuffer,1,sizeof(framebuffer),out); fclose(out);
    }
    return 0;
}

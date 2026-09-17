#include "navigation_icon.h"
#include <cmath>
#include <cstdint>

void drawNavigationIcon(lv_event_t* event) {
    auto* obj = lv_event_get_target(event);
    auto* ctx = lv_event_get_draw_ctx(event);
    const int icon = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    lv_point_t c{static_cast<lv_coord_t>(a.x1 + (a.x2 - a.x1) / 2 - (icon==2 ? 43:52)),
                 static_cast<lv_coord_t>(a.y1 + 25)};
    const auto color = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
    auto stroke = [&](lv_point_t p, lv_point_t q, int width = 2) {
        lv_draw_line_dsc_t dsc;
        lv_draw_line_dsc_init(&dsc);
        dsc.color = color;
        dsc.width = width;
        lv_draw_line(ctx, &dsc, &p, &q);
    };
    auto point = [&](float angle, float radius) {
        return lv_point_t{static_cast<lv_coord_t>(c.x + radius * std::cos(angle)),
                          static_cast<lv_coord_t>(c.y + radius * std::sin(angle))};
    };
    if (icon == 0) {
        for (int i = 0; i < 16; ++i)
            stroke(point(2.35f + i * 4.72f / 16, 12), point(2.35f + (i + 1) * 4.72f / 16, 12));
        for (int i = 0; i < 5; ++i)
            stroke(point(2.35f + i * 4.72f / 4, 8), point(2.35f + i * 4.72f / 4, 12));
        stroke(c, point(-0.9f, 9), 3);
    } else if (icon == 1) {
        stroke({static_cast<lv_coord_t>(c.x - 8), static_cast<lv_coord_t>(c.y + 13)},
               {static_cast<lv_coord_t>(c.x - 8), static_cast<lv_coord_t>(c.y - 12)});
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = color;
        for (int y = 0; y < 3; ++y) for (int x = 0; x < 4; ++x) if ((x + y) % 2 == 0) {
            lv_area_t square{static_cast<lv_coord_t>(c.x - 6 + x * 5),
                static_cast<lv_coord_t>(c.y - 11 + y * 5),
                static_cast<lv_coord_t>(c.x - 2 + x * 5),
                static_cast<lv_coord_t>(c.y - 7 + y * 5)};
            lv_draw_rect(ctx, &dsc, &square);
        }
    } else {
        for (int i = 0; i < 20; ++i)
            stroke(point(i * 6.283f / 20, 8), point((i + 1) * 6.283f / 20, 8));
        for (int i = 0; i < 8; ++i)
            stroke(point(i * 6.283f / 8, 9), point(i * 6.283f / 8, 13), 4);
        for (int i = 0; i < 12; ++i)
            stroke(point(i * 6.283f / 12, 3), point((i + 1) * 6.283f / 12, 3), 1);
    }
}

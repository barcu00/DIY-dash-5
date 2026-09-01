#include "icon_assets.h"

#include <array>
#include <cstddef>
#include <cstdlib>

namespace {
using Mask = std::array<uint16_t, 16>;
constexpr std::size_t kAssetCount = static_cast<std::size_t>(IconId::Count);

void pixel(Mask& m, int x, int y) {
    if (x < 0 || x > 15 || y < 0 || y > 15) return;
    m[static_cast<std::size_t>(y)] |= static_cast<uint16_t>(1U << (15 - x));
}

void hline(Mask& m, int x0, int x1, int y) {
    if (x0 > x1) { const int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; ++x) pixel(m, x, y);
}

void vline(Mask& m, int x, int y0, int y1) {
    if (y0 > y1) { const int t = y0; y0 = y1; y1 = t; }
    for (int y = y0; y <= y1; ++y) pixel(m, x, y);
}

void line(Mask& m, int x0, int y0, int x1, int y1) {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        pixel(m, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void thickLine(Mask& m, int x0, int y0, int x1, int y1) {
    line(m, x0, y0, x1, y1);
    if (std::abs(x1 - x0) >= std::abs(y1 - y0)) line(m, x0, y0 + 1, x1, y1 + 1);
    else line(m, x0 + 1, y0, x1 + 1, y1);
}

void rect(Mask& m, int x0, int y0, int x1, int y1) {
    hline(m, x0, x1, y0);
    hline(m, x0, x1, y1);
    vline(m, x0, y0, y1);
    vline(m, x1, y0, y1);
}

void circle(Mask& m, int cx, int cy, int r) {
    int x = r;
    int y = 0;
    int err = 0;
    while (x >= y) {
        pixel(m, cx + x, cy + y); pixel(m, cx + y, cy + x);
        pixel(m, cx - y, cy + x); pixel(m, cx - x, cy + y);
        pixel(m, cx - x, cy - y); pixel(m, cx - y, cy - x);
        pixel(m, cx + y, cy - x); pixel(m, cx + x, cy - y);
        ++y;
        if (err <= 0) err += 2 * y + 1;
        if (err > 0) { --x; err -= 2 * x + 1; }
    }
}

void dot2(Mask& m, int x, int y) {
    pixel(m, x, y); pixel(m, x + 1, y); pixel(m, x, y + 1); pixel(m, x + 1, y + 1);
}

void drawGauge(Mask& m, int nx, int ny) {
    circle(m, 8, 8, 6);
    hline(m, 3, 4, 8); hline(m, 12, 13, 8);
    vline(m, 8, 2, 3);
    line(m, 4, 4, 5, 5); line(m, 11, 5, 12, 4);
    thickLine(m, 8, 8, nx, ny);
    dot2(m, 7, 7);
}

void drawThermometer(Mask& m) {
    circle(m, 7, 12, 2);
    vline(m, 5, 5, 11); vline(m, 9, 5, 11);
    hline(m, 6, 8, 4);
    thickLine(m, 7, 7, 7, 12);
    hline(m, 11, 14, 6); hline(m, 11, 13, 9);
}

void drawOilCan(Mask& m) {
    rect(m, 2, 6, 9, 12);
    hline(m, 4, 7, 4); vline(m, 5, 4, 6);
    line(m, 9, 7, 12, 5); hline(m, 12, 14, 5);
    line(m, 13, 9, 12, 11); line(m, 12, 11, 13, 13); line(m, 13, 13, 14, 11); line(m, 14, 11, 13, 9);
}

void drawFuelPump(Mask& m) {
    rect(m, 2, 3, 8, 13);
    rect(m, 3, 4, 7, 7);
    hline(m, 1, 9, 14);
    line(m, 8, 5, 11, 6); vline(m, 11, 6, 11); line(m, 11, 11, 13, 12);
    vline(m, 13, 8, 12);
}

void drawChip(Mask& m) {
    rect(m, 4, 4, 11, 11);
    rect(m, 6, 6, 9, 9);
    for (int p = 5; p <= 10; p += 2) {
        pixel(m, p, 2); pixel(m, p, 13); pixel(m, 2, p); pixel(m, 13, p);
        pixel(m, p, 3); pixel(m, p, 12); pixel(m, 3, p); pixel(m, 12, p);
    }
}

void drawWarning(Mask& m) {
    line(m, 8, 2, 2, 13); line(m, 2, 13, 14, 13); line(m, 14, 13, 8, 2);
    vline(m, 8, 6, 9); dot2(m, 7, 11);
}

void drawIcon(IconId id, Mask& m) {
    switch (id) {
        case IconId::Generic:
            line(m, 8, 2, 14, 8); line(m, 14, 8, 8, 14); line(m, 8, 14, 2, 8); line(m, 2, 8, 8, 2); dot2(m, 7, 7); break;
        case IconId::Rpm:
            drawGauge(m, 12, 4); break;
        case IconId::Speed:
            drawGauge(m, 11, 5); hline(m, 0, 3, 11); hline(m, 1, 4, 13); break;
        case IconId::Gear:
            circle(m, 8, 3, 2); thickLine(m, 8, 5, 8, 13); thickLine(m, 4, 7, 12, 7); vline(m, 4, 7, 12); vline(m, 12, 7, 12); break;
        case IconId::Throttle:
            circle(m, 8, 8, 6); thickLine(m, 8, 3, 8, 13); hline(m, 4, 12, 8); break;
        case IconId::Boost:
            circle(m, 6, 9, 4); circle(m, 6, 9, 2); line(m, 9, 7, 12, 4); hline(m, 12, 15, 4); hline(m, 9, 14, 12); break;
        case IconId::Pressure:
            drawGauge(m, 5, 6); break;
        case IconId::Temperature:
            drawThermometer(m); break;
        case IconId::OilPressure:
            drawOilCan(m); break;
        case IconId::OilTemperature:
            drawOilCan(m); vline(m, 11, 2, 4); line(m, 13, 2, 12, 4); line(m, 15, 2, 14, 4); break;
        case IconId::FuelPressure:
            drawGauge(m, 11, 5); line(m, 3, 12, 4, 10); line(m, 4, 10, 5, 12); line(m, 5, 12, 4, 14); line(m, 4, 14, 3, 12); break;
        case IconId::Fuel:
            drawFuelPump(m); break;
        case IconId::Lambda:
            thickLine(m, 5, 13, 8, 3); thickLine(m, 8, 3, 12, 13); hline(m, 2, 6, 13); break;
        case IconId::Injector:
            rect(m, 4, 2, 10, 6); line(m, 5, 6, 10, 11); line(m, 9, 6, 12, 9); hline(m, 9, 12, 11); pixel(m, 8, 13); pixel(m, 11, 13); pixel(m, 13, 12); break;
        case IconId::Ignition:
            line(m, 9, 1, 4, 9); hline(m, 4, 8, 9); line(m, 8, 9, 6, 15); line(m, 6, 15, 13, 7); hline(m, 9, 13, 7); break;
        case IconId::Battery:
            rect(m, 2, 5, 13, 13); hline(m, 4, 6, 3); hline(m, 10, 11, 3); hline(m, 4, 7, 9); vline(m, 10, 7, 11); hline(m, 9, 11, 9); break;
        case IconId::Ecu:
            drawChip(m); break;
        case IconId::Analog:
            line(m, 1, 9, 4, 9); line(m, 4, 9, 6, 5); line(m, 6, 5, 9, 12); line(m, 9, 12, 12, 7); hline(m, 12, 15, 7); break;
        case IconId::Pwm:
            hline(m, 1, 4, 11); vline(m, 4, 5, 11); hline(m, 4, 8, 5); vline(m, 8, 5, 11); hline(m, 8, 12, 11); vline(m, 12, 5, 11); hline(m, 12, 15, 5); break;
        case IconId::Dbw:
            circle(m, 7, 8, 5); thickLine(m, 7, 4, 7, 12); hline(m, 3, 11, 8); line(m, 12, 3, 10, 7); hline(m, 10, 13, 7); line(m, 13, 7, 11, 12); break;
        case IconId::Traction:
            circle(m, 5, 9, 4); line(m, 3, 5, 7, 13); line(m, 8, 5, 13, 4); line(m, 9, 8, 14, 7); line(m, 10, 11, 15, 10); break;
        case IconId::Launch:
            drawGauge(m, 11, 5); vline(m, 13, 2, 7); line(m, 11, 4, 13, 2); line(m, 15, 4, 13, 2); break;
        case IconId::AntiLag:
            circle(m, 6, 9, 4); circle(m, 6, 9, 2); line(m, 9, 7, 12, 4); hline(m, 12, 15, 4); line(m, 12, 10, 10, 13); line(m, 10, 13, 13, 12); line(m, 13, 12, 12, 15); break;
        case IconId::Limiter:
            drawGauge(m, 11, 5); hline(m, 2, 14, 14); hline(m, 4, 12, 12); break;
        case IconId::Brake:
            circle(m, 8, 8, 5); circle(m, 8, 8, 2); vline(m, 1, 5, 11); vline(m, 14, 5, 11); hline(m, 1, 3, 5); hline(m, 12, 14, 11); break;
        case IconId::Fan:
            circle(m, 8, 8, 2); thickLine(m, 8, 6, 5, 2); thickLine(m, 10, 8, 14, 5); thickLine(m, 8, 10, 11, 14); thickLine(m, 6, 8, 2, 11); break;
        case IconId::Nitrous:
            rect(m, 5, 4, 11, 13); hline(m, 7, 9, 2); vline(m, 8, 2, 4); hline(m, 4, 12, 13); line(m, 11, 6, 13, 8); line(m, 13, 8, 11, 10); break;
        case IconId::Can:
            circle(m, 3, 4, 2); circle(m, 13, 12, 2); line(m, 5, 4, 11, 11); line(m, 4, 7, 10, 13); pixel(m, 4, 7); pixel(m, 10, 13); break;
        case IconId::Warning:
            drawWarning(m); break;
        case IconId::Switch:
            rect(m, 2, 5, 13, 11); circle(m, 6, 8, 2); hline(m, 9, 11, 8); break;
        case IconId::Output:
            rect(m, 2, 4, 8, 12); hline(m, 8, 14, 8); line(m, 11, 5, 14, 8); line(m, 14, 8, 11, 11); break;
        case IconId::Starter:
            circle(m, 6, 8, 4); hline(m, 10, 14, 8); line(m, 12, 5, 15, 8); line(m, 15, 8, 12, 11); hline(m, 2, 4, 14); break;
        case IconId::AirConditioning:
            vline(m, 8, 2, 14); hline(m, 2, 14, 8); line(m, 3, 3, 13, 13); line(m, 13, 3, 3, 13); circle(m, 8, 8, 2); break;
        case IconId::Egt:
            hline(m, 2, 10, 10); vline(m, 2, 7, 10); line(m, 10, 10, 13, 12); hline(m, 13, 15, 12); line(m, 5, 7, 4, 4); line(m, 8, 7, 7, 3); line(m, 11, 8, 10, 4); break;
        case IconId::Ethanol:
            line(m, 8, 2, 4, 8); line(m, 4, 8, 5, 12); line(m, 5, 12, 8, 14); line(m, 8, 14, 11, 12); line(m, 11, 12, 12, 8); line(m, 12, 8, 8, 2); line(m, 6, 10, 10, 6); line(m, 7, 11, 11, 7); break;
        case IconId::Knock:
            line(m, 8, 1, 8, 5); line(m, 8, 11, 8, 15); line(m, 1, 8, 5, 8); line(m, 11, 8, 15, 8); line(m, 3, 3, 6, 6); line(m, 10, 10, 13, 13); line(m, 13, 3, 10, 6); line(m, 6, 10, 3, 13); circle(m, 8, 8, 2); break;
        case IconId::FuelPump:
            drawFuelPump(m); line(m, 9, 13, 14, 13); line(m, 12, 11, 14, 13); line(m, 14, 13, 12, 15); break;
        case IconId::SensorError:
            drawThermometer(m); line(m, 11, 2, 15, 6); line(m, 15, 2, 11, 6); break;
        case IconId::GearCut:
            circle(m, 7, 3, 2); vline(m, 7, 5, 13); hline(m, 3, 11, 8); vline(m, 3, 8, 12); vline(m, 11, 8, 12); thickLine(m, 2, 14, 14, 2); break;
        case IconId::Idle:
            drawGauge(m, 5, 10); hline(m, 3, 7, 13); break;
        case IconId::Dsg:
            circle(m, 5, 8, 4); circle(m, 11, 8, 4); vline(m, 8, 3, 13); hline(m, 6, 10, 8); break;
        case IconId::LapTime:
            circle(m, 8, 9, 5); hline(m, 6, 10, 2); vline(m, 8, 2, 4); hline(m, 11, 13, 4); thickLine(m, 8, 9, 11, 6); pixel(m, 8, 9); break;
        case IconId::Invalid:
        case IconId::Count:
            break;
    }
}

struct AssetStorage {
    std::array<Mask, kAssetCount> masks{};
    std::array<IconAsset, kAssetCount> assets{};

    AssetStorage() {
        for (std::size_t i = 1; i < kAssetCount; ++i) {
            drawIcon(static_cast<IconId>(i), masks[i]);
            assets[i] = {masks[i].data(), 16U, 16U, 16U};
        }
    }
};

AssetStorage& storage() {
    static AssetStorage value;
    return value;
}
}

const IconAsset& IconAssets::get(IconId id) {
    const std::size_t index = static_cast<std::size_t>(id);
    AssetStorage& s = storage();
    return index < s.assets.size() ? s.assets[index] : s.assets[0];
}

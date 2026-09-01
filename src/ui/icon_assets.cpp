#include "icon_assets.h"

#include <array>
#include <cstddef>

namespace {
struct Glyph {
    uint8_t row[5];
};

Glyph glyph(char c) {
    switch (c) {
        case 'A': return {{0b010, 0b101, 0b111, 0b101, 0b101}};
        case 'B': return {{0b110, 0b101, 0b110, 0b101, 0b110}};
        case 'C': return {{0b011, 0b100, 0b100, 0b100, 0b011}};
        case 'D': return {{0b110, 0b101, 0b101, 0b101, 0b110}};
        case 'E': return {{0b111, 0b100, 0b110, 0b100, 0b111}};
        case 'F': return {{0b111, 0b100, 0b110, 0b100, 0b100}};
        case 'G': return {{0b011, 0b100, 0b101, 0b101, 0b011}};
        case 'H': return {{0b101, 0b101, 0b111, 0b101, 0b101}};
        case 'I': return {{0b111, 0b010, 0b010, 0b010, 0b111}};
        case 'J': return {{0b001, 0b001, 0b001, 0b101, 0b010}};
        case 'K': return {{0b101, 0b101, 0b110, 0b101, 0b101}};
        case 'L': return {{0b100, 0b100, 0b100, 0b100, 0b111}};
        case 'M': return {{0b101, 0b111, 0b111, 0b101, 0b101}};
        case 'N': return {{0b101, 0b111, 0b111, 0b111, 0b101}};
        case 'O': return {{0b010, 0b101, 0b101, 0b101, 0b010}};
        case 'P': return {{0b110, 0b101, 0b110, 0b100, 0b100}};
        case 'Q': return {{0b010, 0b101, 0b101, 0b111, 0b011}};
        case 'R': return {{0b110, 0b101, 0b110, 0b101, 0b101}};
        case 'S': return {{0b011, 0b100, 0b010, 0b001, 0b110}};
        case 'T': return {{0b111, 0b010, 0b010, 0b010, 0b010}};
        case 'U': return {{0b101, 0b101, 0b101, 0b101, 0b111}};
        case 'V': return {{0b101, 0b101, 0b101, 0b101, 0b010}};
        case 'W': return {{0b101, 0b101, 0b111, 0b111, 0b101}};
        case 'X': return {{0b101, 0b101, 0b010, 0b101, 0b101}};
        case 'Y': return {{0b101, 0b101, 0b010, 0b010, 0b010}};
        case 'Z': return {{0b111, 0b001, 0b010, 0b100, 0b111}};
        default: return {{0, 0, 0, 0, 0}};
    }
}

constexpr std::size_t kAssetCount = static_cast<std::size_t>(IconId::LapTime) + 1U;

const char* const kCodes[kAssetCount] = {
    "", "GN", "RP", "SP", "GE", "TH", "BO", "PR", "TE", "OP", "OT", "FP",
    "FU", "LA", "IN", "IG", "BA", "EC", "AN", "PW", "DB", "TC", "LC", "AL",
    "LI", "BR", "FA", "NO", "CA", "WA", "SW", "OU", "ST", "AC", "LT"
};

void drawGlyph(std::array<uint16_t, 16>& rows, char c, uint8_t x0) {
    const Glyph g = glyph(c);
    for (uint8_t gy = 0; gy < 5; ++gy) {
        for (uint8_t gx = 0; gx < 3; ++gx) {
            if ((g.row[gy] & (1U << (2U - gx))) == 0U) continue;
            const uint8_t px = static_cast<uint8_t>(x0 + gx * 2U);
            const uint8_t py = static_cast<uint8_t>(3U + gy * 2U);
            rows[py] |= static_cast<uint16_t>(1U << (15U - px));
            rows[py] |= static_cast<uint16_t>(1U << (15U - (px + 1U)));
            rows[py + 1U] |= static_cast<uint16_t>(1U << (15U - px));
            rows[py + 1U] |= static_cast<uint16_t>(1U << (15U - (px + 1U)));
        }
    }
}

struct AssetStorage {
    std::array<std::array<uint16_t, 16>, kAssetCount> masks{};
    std::array<IconAsset, kAssetCount> assets{};

    AssetStorage() {
        for (std::size_t i = 1; i < kAssetCount; ++i) {
            drawGlyph(masks[i], kCodes[i][0], 1U);
            drawGlyph(masks[i], kCodes[i][1], 9U);
            masks[i][0] = 0x7FFEU;
            masks[i][15] = 0x7FFEU;
            for (uint8_t y = 1; y < 15; ++y) {
                masks[i][y] |= 0x4002U;
            }
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

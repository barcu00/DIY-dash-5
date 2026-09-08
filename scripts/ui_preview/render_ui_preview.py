import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


EXPECTED_PREVIEWS = (
    "ui-preview-dash.png",
    "ui-preview-track.png",
    "ui-preview-settings-home.png",
    "ui-preview-settings-display.png",
    "ui-preview-settings-can.png",
    "ui-preview-settings-shift.png",
    "ui-preview-settings-units.png",
    "ui-preview-settings-layouts.png",
    "ui-preview-settings-system.png",
    "ui-preview-tile-editor.png",
    "ui-preview-warning.png",
)
ROOT = Path(__file__).resolve().parent
CONTRACT = json.loads((ROOT / "ui_contract.json").read_text(encoding="utf-8"))
C = CONTRACT["colors"]


def font(size, bold=False):
    candidates = (
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold
        else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf",
    )
    for candidate in candidates:
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            pass
    return ImageFont.load_default()


F12, F14, F18, F24, F28 = font(12), font(14), font(18), font(24, True), font(28, True)


def base():
    return Image.new("RGB", (CONTRACT["width"], CONTRACT["height"]), C["background"])


def shift(draw):
    colors = [C["green"]] * 6 + [C["yellow"]] * 3 + [C["red"]] * 3
    for index, color in enumerate(colors):
        x = 8 + index * 66
        draw.rounded_rectangle((x, 8, x + 60, 24), 4, fill=color)


def navigation(draw, active):
    labels = ("DASH", "TRACK", "SETTINGS")
    widths = (267, 266, 267)
    x = 0
    for label, width in zip(labels, widths):
        draw.rectangle((x, 430, x + width, 480), fill="#153B57" if label == active else "#10151B")
        box = draw.textbbox((0, 0), label, font=F14)
        draw.text((x + (width - (box[2] - box[0])) / 2, 447), label, fill=C["text"], font=F14)
        x += width


def tile(draw, rect, title, value, unit="", centered=False, warning=False):
    x, y, w, h = rect
    draw.rounded_rectangle((x, y, x + w, y + h), 8, fill=C["panel"],
                           outline=C["red"] if warning else C["border"], width=3 if warning else 1)
    draw.rounded_rectangle((x, y + 1, x + 4, y + h - 1), 2, fill=C["blue"])
    if centered:
        draw.text((x + w / 2, y + 9), title, fill=C["muted"], font=F12, anchor="ma")
        draw.text((x + w / 2, y + 52), value, fill=C["text"], font=F28, anchor="mm")
        draw.text((x + w - 18, y + 55), unit, fill=C["muted"], font=F12, anchor="rm")
    else:
        draw.text((x + 12, y + 10), title, fill=C["muted"], font=F12)
        draw.text((x + w / 2, y + 54), value, fill=C["text"], font=F24,
                  anchor="mm")
        unit_y = y + h - 19 if title in ("CLT", "OIL TEMP") else y + h - 8
        draw.text((x + w - 8, unit_y), unit, fill=C["muted"], font=F12, anchor="rs")
    if title in ("CLT", "OIL TEMP"):
        temperature = float(value)
        fraction = max(0.0, min(1.0, (temperature - 40.0) / 90.0))
        color = C["blue"] if temperature < 75.0 else (
            C["red"] if temperature >= 130.0 else (
                C["yellow"] if temperature >= 116.25 else C["green"]))
        draw.rounded_rectangle((x + 12, y + h - 11, x + w - 12, y + h - 4),
                               3, fill=C["border"])
        fill_x = x + 12 + int((w - 24) * fraction)
        if fill_x > x + 12:
            draw.rounded_rectangle((x + 12, y + h - 11, fill_x, y + h - 4),
                                   3, fill=color)


def data_page(track=False):
    image = base()
    draw = ImageDraw.Draw(image)
    shift(draw)
    rows = CONTRACT["row_y"]
    left = [("SPEED", "126", "km/h"), ("LAMBDA", "0.88", "lambda"),
            ("CLT", "94", "°C"), ("OIL PRESS", "4.2", "bar")]
    right = [("TPS", "78", "%"), ("IAT", "31", "°C"),
             ("OIL TEMP", "103", "°C"), ("BATTERY", "13.9", "V")]
    for index, entry in enumerate(left):
        tile(draw, (8, rows[index], 176, 90), *entry)
    for index, entry in enumerate(right):
        tile(draw, (616, rows[index], 176, 90), *entry)
    if track:
        centers = [("RPM", "6840", "rpm"), ("GEAR", "4", ""),
                   ("MAP", "1.42", "bar"), ("FUEL PRESS", "3.8", "bar")]
        for index, entry in enumerate(centers):
            tile(draw, (192, rows[index], 416, 90), *entry, centered=True)
        navigation(draw, "TRACK")
    else:
        tile(draw, (192, rows[0], 416, 90), "RPM", "6840", "rpm", centered=True)
        tile(draw, (192, rows[1], 416, 90), "GEAR", "4", "", centered=True)
        small = [("MAP", "1.42", "bar"), ("FUEL PRESS", "3.8", "bar"),
                 ("TPS", "78", "%"), ("BATTERY", "13.9", "V")]
        positions = ((192, rows[2]), (404, rows[2]), (192, rows[3]), (404, rows[3]))
        for (title, value, unit), (x, y) in zip(small, positions):
            tile(draw, (x, y, 204, 90), title, value, unit)
        navigation(draw, "DASH")
    return image


def settings_page():
    image = base(); draw = ImageDraw.Draw(image)
    draw.text((24, 18), "SETTINGS", fill=C["text"], font=F28)
    draw.text((190, 29), "CAN DISABLED  •  DEMO", fill=C["muted"], font=F12)
    draw.rounded_rectangle((16, 66, 784, 414), 8, fill=C["panel"], outline=C["border"])
    draw.text((30, 82), "DISPLAY", fill=C["blue"], font=F14)
    draw.text((30, 113), "Brightness", fill=C["text"], font=F14)
    draw.rounded_rectangle((220, 116, 720, 130), 7, fill=C["border"])
    draw.rounded_rectangle((220, 116, 660, 130), 7, fill=C["blue"])
    draw.text((30, 158), "DATA SOURCE & CAN", fill=C["blue"], font=F14)
    for x, text in ((30, "DEMO ▾"), (220, "500 kbit/s ▾"), (440, "500 ms")):
        draw.rounded_rectangle((x, 184, x + 170, 225), 5, fill="#18222C", outline=C["border"])
        draw.text((x + 12, 196), text, fill=C["text"], font=F14)
    draw.text((30, 252), "SHIFT LIGHTS", fill=C["blue"], font=F14)
    for x, title, value in ((30, "START", "5500"), (270, "RED", "7000"), (510, "MAX", "8000")):
        draw.text((x, 278), title, fill=C["muted"], font=F12)
        draw.rounded_rectangle((x, 298, x + 180, 340), 5, fill="#18222C", outline=C["border"])
        draw.text((x + 18, 307), value, fill=C["text"], font=F18)
    draw.rounded_rectangle((30, 357, 250, 402), 5, fill="#153B57")
    draw.text((140, 380), "SAVE SETTINGS", fill=C["text"], font=F14, anchor="mm")
    navigation(draw, "SETTINGS")
    return image


def settings_shell(title, back=True):
    image = base()
    draw = ImageDraw.Draw(image)
    if back:
        draw.rounded_rectangle((20, 18, 140, 56), 5, fill="#153B57")
        draw.text((80, 37), "< BACK", fill=C["text"], font=F14, anchor="mm")
    draw.text((400, 20), title, fill=C["text"], font=F24, anchor="ma")
    navigation(draw, "SETTINGS")
    return image, draw


def settings_control(draw, box, label, value=""):
    x1, y1, x2, y2 = box
    draw.rounded_rectangle(box, 8, fill=C["panel"], outline=C["border"])
    draw.text((x1 + 14, y1 + 14), label, fill=C["muted"], font=F12)
    if value:
        draw.text(((x1 + x2) / 2, (y1 + y2) / 2 + 12), value,
                  fill=C["text"], font=F18, anchor="mm")


def settings_home_page():
    image, draw = settings_shell("SETTINGS", back=False)
    labels = (("DISPLAY", "Brightness & memory"), ("DATA & CAN", "Source, profile, bitrate"),
              ("SHIFT LIGHT", "Start, redline, maximum"), ("UNITS", "Temperature & pressure"),
              ("LAYOUTS", "Dash & track tiles"), ("SYSTEM", "Reset, version, memory"))
    for index, (title, detail) in enumerate(labels):
        col, row = index % 3, index // 3
        x, y = 22 + col * 256, 72 + row * 166
        draw.rounded_rectangle((x, y, x + 244, y + 154), 10,
                               fill=C["panel"], outline=C["border"])
        draw.text((x + 122, y + 62), title, fill=C["text"], font=F18, anchor="mm")
        draw.text((x + 122, y + 96), detail, fill=C["muted"], font=F12, anchor="mm")
    return image


def settings_display_page():
    image, draw = settings_shell("DISPLAY")
    draw.text((44, 110), "BRIGHTNESS", fill=C["text"], font=F18)
    draw.rounded_rectangle((210, 116, 666, 134), 9, fill=C["border"])
    draw.rounded_rectangle((210, 116, 580, 134), 9, fill=C["blue"])
    draw.text((714, 125), "80%", fill=C["blue"], font=F18, anchor="mm")
    settings_control(draw, (44, 180, 370, 320), "MEMORY", "Free heap 199 KiB")
    settings_control(draw, (394, 180, 756, 320), "DISPLAY", "800 x 480  |  RGB565")
    draw.text((400, 352), "LVGL buffers: 250 KiB PSRAM", fill=C["muted"], font=F14, anchor="ma")
    draw.text((700, 29), "SAVED", fill=C["yellow"], font=F12)
    return image


def settings_can_page():
    image, draw = settings_shell("DATA & CAN")
    items = (("SOURCE", "DEMO"), ("PROFILE", "none"),
             ("BITRATE", "500 kbit/s"), ("TIMEOUT", "500 ms"))
    for index, (label, value) in enumerate(items):
        x = 24 + index * 190
        settings_control(draw, (x, 90, x + 174, 180), label, value)
    settings_control(draw, (24, 216, 776, 360), "CAN STATUS",
                     "DISABLED  |  receive-only  |  RX 0  |  rejected 0")
    return image


def settings_shift_page():
    image, draw = settings_shell("SHIFT LIGHT")
    values = (("START RPM", "5500 RPM", 0.32),
              ("RED RPM", "7000 RPM", 0.43),
              ("FLASH RPM", "7500 RPM", 0.46),
              ("MAX RPM", "8000 RPM", 0.50))
    for index, (label, value, progress) in enumerate(values):
        y = 82 + index * 64
        draw.text((40, y), label, fill=C["muted"], font=F14, anchor="lm")
        draw.rounded_rectangle((176, y - 7, 616, y + 7), 7,
                               fill=C["border"])
        knob_x = 176 + int(440 * progress)
        draw.rounded_rectangle((176, y - 7, knob_x, y + 7), 7,
                               fill=C["blue"])
        draw.ellipse((knob_x - 10, y - 10, knob_x + 10, y + 10),
                     fill=C["text"])
        draw.text((642, y), value, fill=C["text"], font=F14, anchor="lm")
    draw.text((40, 338), "FLASH ENABLED", fill=C["muted"], font=F14,
              anchor="lm")
    draw.rounded_rectangle((176, 323, 232, 353), 15, fill=C["blue"])
    draw.ellipse((204, 325, 230, 351), fill=C["text"])
    draw.text((430, 338), "Saved when leaving this screen", fill=C["muted"],
              font=F12, anchor="lm")
    return image


def settings_units_page():
    image, draw = settings_shell("UNITS")
    items = (("TEMPERATURE", "Celsius"), ("PRESSURE", "bar"),
             ("SPEED", "km/h"), ("MIXTURE", "lambda"))
    for index, (label, value) in enumerate(items):
        x = 24 + index * 190
        settings_control(draw, (x, 116, x + 174, 226), label, value)
    draw.text((400, 300), "Applied to DASH, TRACK and warnings",
              fill=C["muted"], font=F14, anchor="ma")
    return image


def settings_layouts_page():
    image, draw = settings_shell("LAYOUTS")
    draw.rounded_rectangle((28, 72, 194, 112), 6, fill="#153B57")
    draw.rounded_rectangle((210, 72, 376, 112), 6, fill=C["panel"], outline=C["border"])
    draw.text((111, 92), "DASH", fill=C["text"], font=F14, anchor="mm")
    draw.text((293, 92), "TRACK", fill=C["text"], font=F14, anchor="mm")
    slots = (("1", "SPEED"), ("2", "MAP"), ("3", "LAMBDA"),
             ("4", "CLT"), ("5", "RPM"), ("6", "GEAR"))
    for index, (slot, value) in enumerate(slots):
        col, row = index % 2, index // 2
        x, y = 22 + col * 382, 126 + row * 72
        settings_control(draw, (x, y, x + 366, y + 62), f"SLOT {slot}", f"{value}  |  VISIBLE")
    draw.rounded_rectangle((22, 354, 182, 404), 6, fill=C["panel"], outline=C["border"])
    draw.rounded_rectangle((618, 354, 778, 404), 6, fill="#153B57")
    draw.text((102, 379), "< PREVIOUS", fill=C["muted"], font=F14, anchor="mm")
    draw.text((400, 379), "1 / 3", fill=C["text"], font=F14, anchor="mm")
    draw.text((698, 379), "NEXT >", fill=C["text"], font=F14, anchor="mm")
    return image


def settings_system_page():
    image, draw = settings_shell("SYSTEM")
    settings_control(draw, (28, 88, 374, 238), "RUNTIME", "Free heap 199 KiB")
    settings_control(draw, (396, 88, 772, 238), "FIRMWARE", "dashboard-dev")
    for x, text in ((28, "RESET DASH"), (282, "RESET TRACK"), (536, "FACTORY RESET")):
        draw.rounded_rectangle((x, 276, x + 236, 338), 7, fill="#32181A", outline=C["red"])
        draw.text((x + 118, 307), text, fill=C["text"], font=F14, anchor="mm")
    draw.text((400, 376), "Reset requires confirmation", fill=C["yellow"], font=F14, anchor="ma")
    return image


def editor_page():
    image = data_page(False).convert("RGBA")
    shade = Image.new("RGBA", image.size, (0, 0, 0, 145)); image.alpha_composite(shade)
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((10, 6, 790, 474), 10, fill=C["panel"], outline=C["blue"], width=2)
    draw.text((32, 18), "TILE SETTINGS", fill=C["text"], font=F24)
    fields = ((32, 70, "Parameter", "CLT ▾"), (622, 70, "Decimals", "0 ▾"),
              (222, 146, "MIN", "40.0"), (402, 146, "READY", "75.0"),
              (582, 146, "MAX", "130.0"), (202, 272, "Direction", "Above ▾"),
              (342, 272, "Threshold", "110.0"),
              (492, 272, "Hysteresis", "2.0"), (642, 272, "Delay ms", "300"))
    for x, y, title, value in fields:
        draw.text((x, y - 20), title, fill=C["muted"], font=F12)
        draw.rounded_rectangle((x, y, min(x + 150, 726), y + 42), 5, fill="#18222C", outline=C["border"])
        draw.text((x + 10, y + 12), value, fill=C["text"], font=F14)
    draw.text((292, 80), "☑ Visible", fill=C["text"], font=F14)
    draw.text((32, 134), "☑ Temperature bar", fill=C["text"], font=F14)
    draw.text((32, 260), "☑ Enable WARNING", fill=C["text"], font=F14)
    draw.text((32, 365), "Temperature: MIN < READY < MAX. Alarm range: 0.0–999.0.",
              fill=C["muted"], font=F12)
    for x, text, width in ((32, "CANCEL", 180), (572, "SAVE TILE", 190)):
        draw.rounded_rectangle((x, 396, x + width, 444), 5, fill="#153B57")
        draw.text((x + width / 2, 420), text, fill=C["text"], font=F14, anchor="mm")
    return image.convert("RGB")


def warning_page():
    image = data_page(True).convert("RGBA")
    shade = Image.new("RGBA", image.size, (0, 0, 0, 150)); image.alpha_composite(shade)
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((90, 90, 710, 390), 12, fill="#5A0909", outline=C["red"], width=5)
    draw.text((400, 125), "WARNING", fill=C["text"], font=F28, anchor="ma")
    draw.text((400, 178), "Coolant temperature", fill=C["text"], font=F24, anchor="ma")
    draw.text((400, 235), "116.0 °C", fill=C["text"], font=F28, anchor="ma")
    draw.text((400, 276), "Limit: 110.0 °C", fill="#FFD0D0", font=F14, anchor="ma")
    draw.rounded_rectangle((270, 315, 530, 370), 6, fill=C["red"])
    draw.text((400, 343), "ACKNOWLEDGE", fill=C["text"], font=F14, anchor="mm")
    return image.convert("RGB")


def render_all(output):
    output.mkdir(parents=True, exist_ok=True)
    images = (
        data_page(False), data_page(True), settings_home_page(),
        settings_display_page(), settings_can_page(), settings_shift_page(),
        settings_units_page(), settings_layouts_page(), settings_system_page(),
        editor_page(), warning_page(),
    )
    for name, image in zip(EXPECTED_PREVIEWS, images):
        image.save(output / name, format="PNG", optimize=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    render_all(parser.parse_args().output)

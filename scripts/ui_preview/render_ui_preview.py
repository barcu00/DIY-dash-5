import argparse
import json
import math
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
    "ui-preview-flag-tiles.png",
    "ui-preview-flag-editor.png",
    "ui-preview-warning.png",
    "ui-preview-analog-style.png",
    "ui-preview-side-gear.png",
    "ui-preview-strip-style.png",
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
    colors = [C["green"]] * 4 + [C["yellow"]] * 4 + [C["red"]] * 4
    for index, color in enumerate(colors):
        x = 8 + index * 66
        draw.rounded_rectangle((x, 8, x + 60, 24), 4, fill=color)


def navigation(draw, active):
    labels = ("DASH", "TRACK", "SETTINGS")
    widths = (267, 266, 267)
    x = 0
    for index, (label, width) in enumerate(zip(labels, widths)):
        color = C["blue"] if label == active else C["muted"]
        cx, cy = x + width / 2 - 62, 455
        if index == 0:
            draw.arc((cx - 12, cy - 12, cx + 12, cy + 12), 135, 405, fill=color, width=2)
            draw.line((cx, cy, cx + 7, cy - 7), fill=color, width=2)
        elif index == 1:
            draw.line((cx - 10, cy - 12, cx - 10, cy + 12), fill=color, width=2)
            for row in range(3):
                for col in range(4):
                    if (row + col) % 2 == 0:
                        draw.rectangle((cx - 8 + col * 5, cy - 12 + row * 5,
                                        cx - 4 + col * 5, cy - 8 + row * 5), fill=color)
        else:
            draw.ellipse((cx - 9, cy - 9, cx + 9, cy + 9), outline=color, width=2)
            draw.ellipse((cx - 3, cy - 3, cx + 3, cy + 3), outline=color, width=2)
            for tooth in range(8):
                angle = tooth * math.pi / 4
                draw.line((cx + 9 * math.cos(angle), cy + 9 * math.sin(angle),
                           cx + 13 * math.cos(angle), cy + 13 * math.sin(angle)), fill=color, width=3)
        draw.text((x + width / 2 + 16, cy), label, fill=color, font=font(16), anchor="mm")
        if label == active:
            draw.rectangle((x + (width - 176) / 2, 477, x + (width + 176) / 2, 479), fill=color)
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
            C["red"] if temperature >= 115.0 else C["green"])
        draw.rounded_rectangle((x + 12, y + h - 11, x + w - 12, y + h - 4),
                               3, fill=C["border"])
        fill_x = x + 12 + int((w - 24) * fraction)
        if fill_x > x + 12:
            draw.rounded_rectangle((x + 12, y + h - 11, fill_x, y + h - 4),
                                   3, fill=color)


def flag_tile(draw, rect, title, state, accent=None):
    x, y, w, h = rect
    unavailable = state == "UNAVAILABLE"
    active = state == "ON" and accent is not None
    background = "#111820"
    rail = "#27313A"
    pill = "#737E87"
    if active:
        background = {C["yellow"]: "#262214", C["green"]: "#12271D",
                      C["red"]: "#2A1519"}[accent]
        rail = accent
        pill = accent
    draw.rounded_rectangle((x, y, x + w, y + h), 8, fill=background,
                           outline=C["border"], width=1)
    draw.rounded_rectangle((x, y + 8, x + 4, y + h - 8), 2, fill=rail)
    draw.text((x + w / 2, y + 10), title, fill=C["muted"], font=F12,
              anchor="ma")
    if unavailable:
        draw.text((x + w / 2, y + 48), "---", fill=C["muted"], font=F24,
                  anchor="mm")
        draw.text((x + w / 2, y + h - 8), "UNAVAILABLE", fill=C["muted"],
                  font=F12, anchor="ms")
    else:
        left, right = x + w / 2 - 46, x + w / 2 + 46
        draw.rounded_rectangle((left, y + 38, right, y + 70), 14, fill=pill)
        draw.text((x + w / 2, y + 54), state, fill=C["background"],
                  font=F18, anchor="mm")


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


def flag_tiles_page():
    image = base()
    draw = ImageDraw.Draw(image)
    shift(draw)
    rows = CONTRACT["row_y"]
    left = [("SPEED", "126", "km/h"), ("CLT", "94", "°C"),
            ("OIL TEMP", "103", "°C"), ("BATTERY", "13.9", "V")]
    right = [("TPS", "78", "%"), ("MAP", "1.42", "bar"),
             ("OIL PRESS", "4.2", "bar"), ("LAMBDA", "0.88", "lambda")]
    for index, entry in enumerate(left):
        tile(draw, (8, rows[index], 176, 90), *entry)
    for index, entry in enumerate(right):
        tile(draw, (616, rows[index], 176, 90), *entry)
    tile(draw, (192, rows[0], 416, 90), "BMW MS43 STOCK", "6840", "rpm",
         centered=True)
    tile(draw, (192, rows[1], 416, 90), "GEAR", "4", "", centered=True)
    flags = (("CHECK ENGINE", "ON", C["red"]),
             ("ENGINE RUN", "ON", C["green"]),
             ("IDLE", "OFF", None),
             ("LAUNCH", "UNAVAILABLE", None))
    positions = ((192, rows[2]), (404, rows[2]),
                 (192, rows[3]), (404, rows[3]))
    for entry, (x, y) in zip(flags, positions):
        flag_tile(draw, (x, y, 204, 90), *entry)
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
    values = (("RPM SCALE MAX", "10000 RPM", 1.00),
              ("YELLOW FROM", "5500 RPM", 0.55),
              ("RED FROM", "7000 RPM", 0.70),
              ("FLASH FROM", "7500 RPM", 0.75),
              ("12-LED FILL MAX", "8000 RPM", 0.80))
    for index, (label, value, progress) in enumerate(values):
        y = 78 + index * 49
        draw.text((40, y), label, fill=C["muted"], font=F14, anchor="lm")
        draw.rounded_rectangle((176, y - 7, 616, y + 7), 7,
                               fill=C["border"])
        knob_x = 176 + int(440 * progress)
        draw.rounded_rectangle((176, y - 7, knob_x, y + 7), 7,
                               fill=C["blue"])
        draw.ellipse((knob_x - 10, y - 10, knob_x + 10, y + 10),
                     fill=C["text"])
        draw.text((642, y), value, fill=C["text"], font=F14, anchor="lm")
    draw.text((40, 330), "FLASH ENABLED", fill=C["muted"], font=F14,
              anchor="lm")
    draw.rounded_rectangle((176, 315, 232, 345), 15, fill=C["blue"])
    draw.ellipse((204, 317, 230, 343), fill=C["text"])
    draw.text((430, 330), "4 Hz full-strip blink", fill=C["muted"],
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
    for x, label, preset in ((28, "DASH", "Analog Style"), (408, "TRACK", "Strip Style")):
        draw.text((x, 85), label, fill=C["text"], font=F12, anchor="lm")
        draw.rounded_rectangle((x + 60, 66, x + 358, 104), 5, fill=C["panel"], outline=C["border"])
        draw.text((x + 74, 85), preset + "  ▾", fill=C["text"], font=F14, anchor="lm")
    for x, label in ((28, "DASH TILES"), (156, "TRACK TILES")):
        draw.rounded_rectangle((x, 110, x + 120, 146), 5, fill="#153B57")
        draw.text((x + 60, 128), label, fill=C["text"], font=F12, anchor="mm")
    draw.text((296, 127), "RPM SCALE MAX: 10000", fill=C["text"], font=F12, anchor="lm")
    draw.rounded_rectangle((570, 121, 752, 137), 8, fill=C["blue"])
    draw.ellipse((744, 119, 764, 139), fill=C["text"])
    slots = (("1", "SPEED"), ("2", "MAP"), ("3", "LAMBDA"),
             ("4", "CLT"), ("5", "RPM"), ("6", "GEAR"))
    for index, (slot, value) in enumerate(slots):
        col, row = index % 2, index // 2
        x, y = 28 + col * 376, 158 + row * 58
        draw.rounded_rectangle((x, y, x + 364, y + 50), 6, fill=C["panel"], outline=C["border"])
        draw.text((x + 12, y + 15), f"SLOT {slot}", fill=C["muted"], font=F12, anchor="lm")
        draw.text((x + 12, y + 35), f"{value}  |  VISIBLE", fill=C["text"], font=F14, anchor="lm")
    draw.rounded_rectangle((28, 352, 188, 396), 6, fill=C["panel"], outline=C["border"])
    draw.rounded_rectangle((612, 352, 772, 396), 6, fill="#153B57")
    draw.text((108, 374), "< PREVIOUS", fill=C["muted"], font=F14, anchor="mm")
    draw.text((400, 374), "1 / 1", fill=C["text"], font=F14, anchor="mm")
    draw.text((692, 374), "NEXT >", fill=C["text"], font=F14, anchor="mm")
    return image


def preset_page(layout, maximum=10000, rpm=6840):
    """Deterministic geometry preview; not a framebuffer capture from LVGL."""
    image = base()
    draw = ImageDraw.Draw(image)

    def hero(rect, title, value, unit="", gear=False):
        x, y, w, h = rect
        draw.rounded_rectangle((x, y, x + w, y + h), 8, fill=C["panel"], outline=C["border"])
        draw.text((x + w / 2, y + 26), title, fill=C["text"], font=font(20), anchor="mm")
        size = 80 if gear else 64 if w >= 260 else 48
        draw.text((x + w / 2, y + h / 2), value, fill=C["text"], font=font(size, True), anchor="mm")
        draw.text((x + w / 2, y + h - 27), unit, fill=C["text"], font=font(20), anchor="mm")

    def rpm_bar(x, y, w, curved=False):
        fraction = max(0, min(1, rpm / maximum))
        for i in range(36):
            a, b = i / 36, (i + 1) / 36
            x1, x2 = x + 4 + int((w - 8) * a), x + 2 + int((w - 8) * b)
            y1 = y + (int(12 + 46 * (2 * a - 1) ** 2) if curved else 24)
            y2 = y + (int(12 + 46 * (2 * b - 1) ** 2) if curved else 24)
            draw.polygon(((x1, y1), (x2, y2), (x2, y2 + 26), (x1, y1 + 26)), fill="#202830")
            lit = max(0, min(1, (fraction - a) / (b - a)))
            if lit:
                xe, ye = x1 + (x2 - x1) * lit, y1 + (y2 - y1) * lit
                color = C["green"] if i < 12 else C["yellow"] if i < 24 else C["red"]
                draw.polygon(((x1, y1), (xe, ye), (xe, ye + 26), (x1, y1 + 26)), fill=color)
        for i in range(11):
            f = i / 10
            tx = max(x + 28, min(x + w - 28, x + 4 + int((w - 8) * f)))
            ty = y + (int(12 + 46 * (2 * f - 1) ** 2) + 55 if curved else 79)
            draw.text((tx, ty), f"{maximum * f / 1000:g}", fill=C["text"], font=F12, anchor="mm")
        draw.text((x + w - 72, y + 8), "RPM x1000", fill=C["text"], font=F12, anchor="mm")

    if layout == "analog":
        for i in range(12):
            color = C["green"] if i < 4 else C["yellow"] if i < 8 else C["red"]
            draw.rounded_rectangle((8 + i * 34, 8, 36 + i * 34, 24), 4, fill=color)
        cx, cy = 216, 226
        def polar(radius, fraction):
            angle = math.radians(135 + 270 * fraction)
            return cx + radius * math.cos(angle), cy + radius * math.sin(angle)
        for i in range(51):
            f = i / 50
            draw.line((*polar(178, f), *polar(164 if i % 5 == 0 else 171, f)),
                      fill=C["red"] if f >= .8 else C["text"], width=3 if i % 5 == 0 else 1)
            if i % 5 == 0:
                draw.text(polar(142, f), f"{maximum * f / 1000:g}", fill=C["text"], font=font(16), anchor="mm")
        draw.line((cx, cy, *polar(158, max(0, min(1, rpm / maximum)))), fill=C["red"], width=6)
        draw.ellipse((cx - 8, cy - 8, cx + 8, cy + 8), fill=C["border"])
        draw.text((cx, 350), str(rpm), fill=C["text"], font=font(48, True), anchor="mm")
        draw.text((cx, 396), "RPM", fill=C["text"], font=font(20), anchor="mm")
        rows = (("SPEED", "137", "km/h"), ("OIL PRESS", "4.8", "bar"),
                ("OIL TEMP", "108", "°C"), ("CLT", "91", "°C"),
                ("MAP", "1.18", "bar"), ("LAMBDA", "0.86", ""))
        for i, (title, value, unit) in enumerate(rows):
            y = 36 + i * 64
            draw.rounded_rectangle((432, y, 792, y + 58), 8, fill=C["panel"], outline=C["border"])
            draw.rectangle((432, y + 1, 436, y + 57), fill=C["blue"])
            draw.text((442, y + 24), title, fill=C["text"], font=F12, anchor="lm")
            draw.text((740, y + 26), value, fill=C["text"], font=F28, anchor="rm")
            draw.text((782, y + 43), unit, fill=C["muted"], font=F12, anchor="rm")
            if title in ("OIL TEMP", "CLT"):
                draw.rectangle((444, y + 51, 780, y + 55), fill=C["border"])
                draw.rectangle((444, y + 51, 444 + int(336 * (float(value) - 40) / 90), y + 55), fill=C["green"])
    elif layout == "side":
        rpm_bar(142, 8, 650)
        hero((8, 8, 126, 318), "GEAR", "3", gear=True)
        hero((142, 112, 320, 214), "RPM", str(rpm), "rpm")
        hero((470, 112, 322, 214), "SPEED", "137", "km/h")
        for i, entry in enumerate((("OIL PRESS", "4.8", "bar"), ("OIL TEMP", "108", "°C"),
                                    ("CLT", "91", "°C"), ("MAP", "1.18", "bar"), ("LAMBDA", "0.86", ""))):
            tile(draw, (8 + 158 * i, 334, 150, 88), *entry)
    else:
        rpm_bar(8, 8, 784, curved=True)
        hero((174, 158, 224, 264), "RPM", str(rpm), "rpm")
        hero((406, 158, 224, 264), "SPEED", "137", "km/h")
        for x, width, entries in ((8, 158, (("OIL PRESS", "4.8", "bar"), ("OIL TEMP", "108", "°C"), ("FUEL PRESS", "4.1", "bar"))),
                                  (638, 154, (("CLT", "91", "°C"), ("IAT", "32", "°C"), ("LAMBDA", "0.86", "")))):
            for i, entry in enumerate(entries):
                tile(draw, (x, 158 + i * 90, width, 84), *entry)
    navigation(draw, "TRACK" if layout == "strip" else "DASH")
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
    image = base().convert("RGBA")
    draw = ImageDraw.Draw(image)
    draw.text((20, 10), "TILE SETTINGS", fill=C["text"], font=F24)
    fields = ((20, 66, 360, "PARAMETER", "CLT ▾"),
              (610, 66, 150, "DECIMALS", "0 ▾"),
              (190, 138, 125, "MIN", "+040.0"),
              (335, 138, 125, "READY", "+075.0"),
              (480, 138, 125, "RED", "+115.0"),
              (625, 138, 125, "MAX", "+130.0"),
              (190, 264, 120, "Direction", "Above ▾"),
              (330, 264, 130, "Threshold", "110.0"),
              (480, 264, 130, "Hysteresis", "002.0"),
              (630, 264, 120, "Delay ms", "300"))
    for x, y, width, title, value in fields:
        draw.text((x, y - 20), title, fill=C["muted"], font=F12)
        draw.rounded_rectangle((x, y, x + width, y + 42), 5,
                               fill="#18222C", outline=C["border"])
        draw.text((x + 10, y + 12), value, fill=C["text"], font=F14)
    for x, y, button_width in ((190, 180, 58), (335, 180, 58),
                               (480, 180, 58), (625, 180, 58),
                               (330, 306, 60),
                               (480, 306, 60), (630, 306, 55)):
        for offset, label in ((0, "−"), (button_width + 6, "+")):
            left = x + offset
            draw.rounded_rectangle((left, y, left + button_width, y + 34), 5,
                                   fill="#153B57")
            draw.text((left + button_width / 2, y + 17), label,
                      fill=C["text"], font=F18, anchor="mm")
    draw.text((410, 74), "☑ Visible", fill=C["text"], font=F14)
    draw.text((20, 126), "☑ Temperature bar", fill=C["text"], font=F14)
    draw.text((20, 252), "☑ Enable WARNING", fill=C["text"], font=F14)
    draw.text((20, 350), "Order: MIN < READY < RED <= MAX",
              fill=C["muted"], font=F12)
    draw.text((20, 368),
              "Temperature: -999.0 to 999.0 | Warning: 0.0 to 999.0",
              fill=C["muted"], font=F12)
    for x, text, width in ((20, "CANCEL", 180), (590, "SAVE TILE", 190)):
        draw.rounded_rectangle((x, 420, x + width, 468), 5, fill="#153B57")
        draw.text((x + width / 2, 444), text, fill=C["text"], font=F14, anchor="mm")
    return image.convert("RGB")


def flag_editor_page():
    image = base()
    draw = ImageDraw.Draw(image)
    draw.text((20, 10), "TILE SETTINGS", fill=C["text"], font=F24)
    draw.text((20, 48), "PARAMETER", fill=C["muted"], font=F12)
    draw.rounded_rectangle((20, 66, 380, 108), 5, fill="#18222C",
                           outline=C["border"])
    draw.text((32, 78), "CHECK ENGINE ▾", fill=C["text"], font=F14)
    draw.text((410, 80), "☑ Visible", fill=C["text"], font=F14)
    draw.text((250, 172), "ACTIVE COLOR", fill=C["muted"], font=F14)
    draw.rounded_rectangle((250, 200, 550, 248), 6, fill="#18222C",
                           outline=C["border"])
    draw.text((270, 216), "RED ▾", fill=C["text"], font=F14)
    draw.text((400, 292),
              "OFF stays neutral. ON uses the selected accent color.",
              fill=C["text"], font=F14, anchor="ma")
    for x, text, width in ((20, "CANCEL", 180), (590, "SAVE TILE", 190)):
        draw.rounded_rectangle((x, 420, x + width, 468), 5, fill="#153B57")
        draw.text((x + width / 2, 444), text, fill=C["text"], font=F14,
                  anchor="mm")
    return image


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
        editor_page(), flag_tiles_page(), flag_editor_page(), warning_page(),
        preset_page("analog"), preset_page("side"), preset_page("strip"),
    )
    for name, image in zip(EXPECTED_PREVIEWS, images):
        image.save(output / name, format="PNG", optimize=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    render_all(parser.parse_args().output)

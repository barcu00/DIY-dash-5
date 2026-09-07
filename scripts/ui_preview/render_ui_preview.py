import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


EXPECTED_PREVIEWS = (
    "ui-preview-dash.png",
    "ui-preview-track.png",
    "ui-preview-settings.png",
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
        draw.text((x + w - 8, y + h - 8), unit, fill=C["muted"], font=F12, anchor="rs")


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


def editor_page():
    image = data_page(False).convert("RGBA")
    shade = Image.new("RGBA", image.size, (0, 0, 0, 145)); image.alpha_composite(shade)
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((40, 35, 760, 445), 10, fill=C["panel"], outline=C["blue"], width=2)
    draw.text((64, 55), "TILE SETTINGS", fill=C["text"], font=F24)
    fields = ((64, 116, "Parameter", "RPM ▾"), (524, 116, "Decimals", "0 ▾"),
              (64, 236, "Direction", "Above ▾"), (234, 236, "Threshold", "7500.00"),
              (404, 236, "Hysteresis", "200.00"), (574, 236, "Delay ms", "300"))
    for x, y, title, value in fields:
        draw.text((x, y - 20), title, fill=C["muted"], font=F12)
        draw.rounded_rectangle((x, y, min(x + 150, 726), y + 42), 5, fill="#18222C", outline=C["border"])
        draw.text((x + 10, y + 12), value, fill=C["text"], font=F14)
    draw.text((350, 125), "☑ Visible", fill=C["text"], font=F14)
    draw.text((64, 190), "☑ Enable WARNING", fill=C["text"], font=F14)
    for x, text in ((64, "CANCEL"), (556, "SAVE TILE")):
        draw.rounded_rectangle((x, 370, x + 160, 420), 5, fill="#153B57")
        draw.text((x + 80, 395), text, fill=C["text"], font=F14, anchor="mm")
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
    images = (data_page(False), data_page(True), settings_page(), editor_page(), warning_page())
    for name, image in zip(EXPECTED_PREVIEWS, images):
        image.save(output / name, format="PNG", optimize=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    render_all(parser.parse_args().output)

from pathlib import Path
import sys
import tempfile
import unittest

SCRIPT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_ROOT / "ui_preview"))
import render_ui_preview  # noqa: E402
from render_ui_preview import EXPECTED_PREVIEWS, render_all  # noqa: E402
from PIL import Image  # noqa: E402

SETTINGS_PREVIEWS = (
    "ui-preview-settings-home.png",
    "ui-preview-settings-display.png",
    "ui-preview-settings-can.png",
    "ui-preview-settings-shift.png",
    "ui-preview-settings-units.png",
    "ui-preview-settings-layouts.png",
    "ui-preview-settings-system.png",
)


class PreviewContractTest(unittest.TestCase):
    def test_renderer_creates_all_800x480_views(self):
        with tempfile.TemporaryDirectory() as output:
            render_all(Path(output))
            for name in EXPECTED_PREVIEWS:
                image = Image.open(Path(output) / name)
                self.assertEqual((800, 480), image.size)

    def test_renderer_creates_every_fixed_settings_screen(self):
        with tempfile.TemporaryDirectory() as output:
            render_all(Path(output))
            for name in SETTINGS_PREVIEWS:
                image = Image.open(Path(output) / name)
                self.assertEqual((800, 480), image.size)

    def test_settings_contract_fits_without_scrolling(self):
        settings = render_ui_preview.CONTRACT["settings"]

        self.assertEqual(348, settings["content_height"])
        self.assertFalse(settings["scrollable"])
        self.assertEqual(6, settings["category_count"])
        self.assertEqual(6, settings["layout_slots_per_page"])
        self.assertEqual(4, settings["shift_slider_count"])
        self.assertTrue(settings["shift_flash_switch"])
        self.assertEqual(100, settings["shift_step_rpm"])

    def test_shift_settings_uses_sliders_without_step_buttons(self):
        source = SCRIPT_ROOT.parent / "src/ui/ui.cpp"
        text = source.read_text(encoding="utf-8")

        shift_section = text.split("void Ui::createShiftSettings", 1)[1]
        shift_section = shift_section.split("void Ui::createUnitSettings", 1)[0]
        self.assertIn("lv_slider_create", shift_section)
        self.assertIn('"FLASH RPM"', shift_section)
        self.assertIn('"FLASH ENABLED"', shift_section)
        self.assertNotIn("makeSpinbox", shift_section)
        self.assertNotIn("ShiftStartDecrease", shift_section)


if __name__ == "__main__":
    unittest.main()

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


if __name__ == "__main__":
    unittest.main()

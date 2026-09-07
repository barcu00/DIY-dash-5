from pathlib import Path
import sys
import tempfile
import unittest

SCRIPT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_ROOT / "ui_preview"))
from render_ui_preview import EXPECTED_PREVIEWS, render_all  # noqa: E402
from PIL import Image  # noqa: E402


class PreviewContractTest(unittest.TestCase):
    def test_renderer_creates_all_800x480_views(self):
        with tempfile.TemporaryDirectory() as output:
            render_all(Path(output))
            for name in EXPECTED_PREVIEWS:
                image = Image.open(Path(output) / name)
                self.assertEqual((800, 480), image.size)


if __name__ == "__main__":
    unittest.main()

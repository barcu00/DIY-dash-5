import csv
import json
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
FLASH_BYTES = 8 * 1024 * 1024


class WaveshareSevenBuildTests(unittest.TestCase):
    def test_platformio_exposes_buildable_seven_inch_environment(self):
        result = subprocess.run(
            ["pio", "project", "config", "--json-output"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
        )

        self.assertIn('"env:waveshare_7"', result.stdout)
        self.assertIn('"waveshare_esp32_s3_touch_lcd_7"', result.stdout)
        self.assertIn('"partitions_diy_dash_8mb.csv"', result.stdout)

    def test_can_pins_are_not_claimed_by_native_usb_cdc_at_startup(self):
        seven = json.loads(
            (ROOT / "boards" / "waveshare_esp32_s3_touch_lcd_7.json")
            .read_text(encoding="utf-8")
        )
        five = json.loads(
            (ROOT / "boards" / "waveshare_esp32_s3_touch_lcd_5.json")
            .read_text(encoding="utf-8")
        )

        seven_flags = seven["build"]["extra_flags"]
        five_flags = five["build"]["extra_flags"]
        self.assertIn("-DARDUINO_USB_CDC_ON_BOOT=0", seven_flags)
        self.assertNotIn("-DARDUINO_USB_CDC_ON_BOOT=1", seven_flags)
        self.assertIn("-DARDUINO_USB_CDC_ON_BOOT=1", five_flags)

    def test_eight_megabyte_layout_fits_and_keeps_large_application(self):
        path = ROOT / "partitions_diy_dash_8mb.csv"
        with path.open(newline="", encoding="utf-8") as source:
            rows = [
                [cell.strip() for cell in row]
                for row in csv.reader(
                    line for line in source
                    if line.strip() and not line.lstrip().startswith("#")
                )
            ]

        partitions = {
            row[0]: (int(row[3], 0), int(row[4], 0), row[1], row[2])
            for row in rows
        }
        ordered = sorted(partitions.items(), key=lambda item: item[1][0])

        self.assertGreaterEqual(partitions["app0"][1], 0x5F0000)
        self.assertEqual((0x600000, 0x80000), partitions["dashcfg"][:2])
        self.assertEqual("factory", partitions["app0"][3])
        for (_, (offset, size, _, _)), (_, (next_offset, _, _, _)) in zip(
            ordered, ordered[1:]
        ):
            self.assertLessEqual(offset + size, next_offset)
        last_offset, last_size, _, _ = ordered[-1][1]
        self.assertLessEqual(last_offset + last_size, FLASH_BYTES)


if __name__ == "__main__":
    unittest.main()

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCREENSHOTS = (
    ROOT / "docs/ui/screenshots/racechrono-connection.png",
    ROOT / "docs/ui/screenshots/racechrono-channels.png",
    ROOT / "docs/ui/screenshots/racechrono-picker.png",
)
REQUIRED_TEXT = (
    "DIY DASH RC",
    "RaceChrono Settings",
    "Add other device",
    "RaceChrono DIY",
    "Bluetooth LE",
    "Monitor",
    "CONNECTION",
    "CHANNELS",
    "CAN / DEMO remains active",
    "0x1FF8",
    "DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin",
    "dashcfg",
    "512 KiB",
    "flash at 0x0",
    "clears existing settings",
    "EXIT WITHOUT SAVE",
    "BLE: WAITING FOR APP",
    "GPS: NO FIX",
)


class RaceChronoDocumentationContractTest(unittest.TestCase):
    def test_readme_and_setup_cover_complete_pairing_and_firmware_flow(self):
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        setup_path = ROOT / "docs/racechrono/setup.md"
        self.assertTrue(setup_path.exists(), "RaceChrono setup guide is missing")
        setup = setup_path.read_text(encoding="utf-8")

        combined = readme + "\n" + setup
        for required in REQUIRED_TEXT:
            self.assertIn(required, combined)

    def test_all_approved_racechrono_screenshots_are_versioned(self):
        for screenshot in SCREENSHOTS:
            self.assertTrue(screenshot.exists(), f"Missing {screenshot}")
            self.assertGreater(screenshot.stat().st_size, 1000)


if __name__ == "__main__":
    unittest.main()

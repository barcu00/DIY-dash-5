import json
from pathlib import Path
import runpy
import tempfile
import unittest


class FakeEnvironment(dict):
    def __init__(self, root):
        super().__init__()
        self.build = root / "build"
        self.build.mkdir()
        self.framework = root / "framework"
        self.framework.mkdir()
        for name in ("bootloader.bin", "partitions.bin", "firmware.bin"):
            (self.build / name).write_bytes(name.encode())
        self.boot_app = self.framework / "boot_app0.bin"
        self.boot_app.write_bytes(b"framework boot app")
        self["FLASH_EXTRA_IMAGES"] = [
            ("0x0000", str(self.build / "bootloader.bin")),
            ("0x8000", str(self.build / "partitions.bin")),
            ("0xe000", str(self.boot_app)),
        ]
        self.command = None
        self.result = 0

    def BoardConfig(self):
        return {"build.mcu": "esp32s3", "upload.flash_size": "16MB"}

    def subst(self, value):
        return (value.replace("$BUILD_DIR", str(self.build))
                .replace("${PROGNAME}", "firmware")
                .replace("$ESP32_APP_OFFSET", "0x10000"))

    def Execute(self, command):
        self.command = command
        return self.result

    def AddPostAction(self, target, action):
        self.action = action


class MergeFirmwareTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.env = FakeEnvironment(Path(self.temp.name))
        script = Path(__file__).resolve().parents[1] / "merge_bin.py"
        runpy.run_path(str(script), init_globals={
            "env": self.env, "Import": lambda _: None,
        })

    def test_external_framework_image_is_available_beside_manifest(self):
        self.env.action([], [], self.env)
        manifest = json.loads((self.env.build / "flash-layout.json").read_text())
        self.assertEqual(["0x0000", "0x8000", "0xe000", "0x10000"],
                         [image["offset"] for image in manifest["images"]])
        for image in manifest["images"]:
            self.assertTrue((self.env.build / image["file"]).is_file(), image)
        self.assertEqual(b"framework boot app",
                         (self.env.build / "boot_app0.bin").read_bytes())

    def test_missing_input_stops_merge(self):
        self.env.boot_app.unlink()
        with self.assertRaisesRegex(RuntimeError, "image.*is missing"):
            self.env.action([], [], self.env)

    def test_esptool_failure_is_not_reported_as_success(self):
        self.env.result = 1
        with self.assertRaisesRegex(RuntimeError, "Failed to create"):
            self.env.action([], [], self.env)
        self.assertFalse((self.env.build / "flash-layout.json").exists())


if __name__ == "__main__":
    unittest.main()

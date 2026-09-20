import csv
import pathlib
import struct
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class PartitionLayoutTests(unittest.TestCase):
    def test_dedicated_configuration_partition_has_future_capacity(self):
        with (ROOT / "partitions_diy_dash.csv").open(
            newline="", encoding="utf-8"
        ) as source:
            rows = {}
            for row in csv.reader(
                line
                for line in source
                if line.strip() and not line.lstrip().startswith("#")
            ):
                cells = [cell.strip() for cell in row]
                rows[cells[0]] = cells

        self.assertEqual("0xC90000", rows["dashcfg"][3])
        self.assertEqual("0x80000", rows["dashcfg"][4])
        self.assertEqual("nvs", rows["dashcfg"][2])
        self.assertEqual("0xD10000", rows["spiffs"][3])
        self.assertEqual("0x2E0000", rows["spiffs"][4])

    def test_platformio_selects_the_custom_table(self):
        text = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        waveshare = text.split("[env:waveshare_5]", 1)[1]
        self.assertIn(
            "board_build.partitions = partitions_diy_dash.csv", waveshare
        )

    def test_generated_partition_parser_accepts_the_required_layout(self):
        from scripts.validate_partition_bin import validate_partition_table

        entries = [
            (0x01, 0x02, 0x009000, 0x005000, "nvs"),
            (0x01, 0x00, 0x00E000, 0x002000, "otadata"),
            (0x00, 0x10, 0x010000, 0x640000, "app0"),
            (0x00, 0x11, 0x650000, 0x640000, "app1"),
            (0x01, 0x02, 0xC90000, 0x080000, "dashcfg"),
            (0x01, 0x82, 0xD10000, 0x2E0000, "spiffs"),
            (0x01, 0x03, 0xFF0000, 0x010000, "coredump"),
        ]
        data = b"".join(self._entry(*entry) for entry in entries)
        data += b"\xff" * 32

        table = validate_partition_table(data)

        self.assertEqual((0x01, 0x02, 0xC90000, 0x080000),
                         table["dashcfg"])

    def test_generated_partition_parser_rejects_small_dashcfg(self):
        from scripts.validate_partition_bin import validate_partition_table

        entries = [
            (0x01, 0x02, 0x009000, 0x005000, "nvs"),
            (0x01, 0x00, 0x00E000, 0x002000, "otadata"),
            (0x00, 0x10, 0x010000, 0x640000, "app0"),
            (0x00, 0x11, 0x650000, 0x640000, "app1"),
            (0x01, 0x02, 0xC90000, 0x005000, "dashcfg"),
            (0x01, 0x82, 0xCE0000, 0x310000, "spiffs"),
            (0x01, 0x03, 0xFF0000, 0x010000, "coredump"),
        ]
        data = b"".join(self._entry(*entry) for entry in entries)
        data += b"\xff" * 32

        with self.assertRaisesRegex(ValueError, "dashcfg"):
            validate_partition_table(data)

    @staticmethod
    def _entry(part_type, subtype, offset, size, label):
        return struct.pack(
            "<HBBII16sI",
            0x50AA,
            part_type,
            subtype,
            offset,
            size,
            label.encode("ascii").ljust(16, b"\0"),
            0,
        )


if __name__ == "__main__":
    unittest.main()

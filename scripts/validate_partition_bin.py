#!/usr/bin/env python3
"""Validate the generated ESP32 partition table used by DIY Dash."""

from pathlib import Path
import struct
import sys


ENTRY = struct.Struct("<HBBII16sI")
PARTITION_MAGIC = 0x50AA
EXPECTED = {
    "nvs": (0x01, 0x02, 0x009000, 0x005000),
    "otadata": (0x01, 0x00, 0x00E000, 0x002000),
    "app0": (0x00, 0x10, 0x010000, 0x640000),
    "app1": (0x00, 0x11, 0x650000, 0x640000),
    "dashcfg": (0x01, 0x02, 0xC90000, 0x080000),
    "spiffs": (0x01, 0x82, 0xD10000, 0x2E0000),
    "coredump": (0x01, 0x03, 0xFF0000, 0x010000),
}


def parse_partition_table(data: bytes) -> dict[str, tuple[int, int, int, int]]:
    table = {}
    for offset in range(0, len(data) - ENTRY.size + 1, ENTRY.size):
        magic, part_type, subtype, address, size, raw_label, _ = ENTRY.unpack_from(
            data, offset
        )
        if magic != PARTITION_MAGIC:
            break
        label = raw_label.split(b"\0", 1)[0].decode("ascii")
        if not label or label in table:
            raise ValueError(f"invalid or duplicate partition label: {label!r}")
        table[label] = (part_type, subtype, address, size)
    return table


def validate_partition_table(
    data: bytes,
) -> dict[str, tuple[int, int, int, int]]:
    table = parse_partition_table(data)
    for label, expected in EXPECTED.items():
        actual = table.get(label)
        if actual != expected:
            raise ValueError(
                f"partition {label} mismatch: expected {expected}, got {actual}"
            )
    return table


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: validate_partition_bin.py PARTITIONS.BIN", file=sys.stderr)
        return 2
    path = Path(argv[1])
    try:
        table = validate_partition_table(path.read_bytes())
    except (OSError, UnicodeDecodeError, ValueError) as error:
        print(f"partition validation failed: {error}", file=sys.stderr)
        return 1
    _, _, offset, size = table["dashcfg"]
    print(f"dashcfg offset=0x{offset:X} size=0x{size:X}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

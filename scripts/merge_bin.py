Import("env")

import json
import os
from os.path import basename, join

app_bin = join("$BUILD_DIR", "${PROGNAME}.bin")
merged_bin = join("$BUILD_DIR", "DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin")
manifest_file = join("$BUILD_DIR", "flash-layout.json")


def merge_firmware(source, target, env):
    board = env.BoardConfig()
    image_pairs = [
        (str(image[0]), env.subst(str(image[1])))
        for image in env.get("FLASH_EXTRA_IMAGES", [])
    ]
    image_pairs.append((env.subst("$ESP32_APP_OFFSET"), env.subst(app_bin)))

    for address, image_path in image_pairs:
        if not os.path.isfile(image_path):
            raise RuntimeError(
                "Cannot merge firmware: image at {} is missing: {}".format(
                    address, image_path
                )
            )

    flash_images = []
    for address, image_path in image_pairs:
        flash_images.extend([address, '"{}"'.format(image_path)])

    command = " ".join(
        [
            '"$PYTHONEXE"',
            '"$OBJCOPY"',
            "--chip",
            board.get("build.mcu", "esp32s3"),
            "merge_bin",
            "--fill-flash-size",
            board.get("upload.flash_size", "16MB"),
            "-o",
            merged_bin,
        ]
        + flash_images
    )
    result = env.Execute(command)
    if result != 0:
        raise RuntimeError("Failed to create merged DIY Dash flash image")

    manifest = {
        "chip": board.get("build.mcu", "esp32s3"),
        "flash_size": board.get("upload.flash_size", "16MB"),
        "merged_image": basename(env.subst(merged_bin)),
        "images": [
            {"offset": address, "file": basename(image_path)}
            for address, image_path in image_pairs
        ],
    }
    with open(env.subst(manifest_file), "w", encoding="utf-8") as output:
        json.dump(manifest, output, indent=2)
        output.write("\n")


env.AddPostAction(app_bin, merge_firmware)

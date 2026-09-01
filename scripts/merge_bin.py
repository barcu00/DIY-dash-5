Import("env")

from os.path import join

app_bin = join("$BUILD_DIR", "${PROGNAME}.bin")
merged_bin = join("$BUILD_DIR", "OpenDash-v0.2-full.bin")


def merge_firmware(source, target, env):
    board = env.BoardConfig()
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", app_bin]
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
        raise RuntimeError("Failed to create merged OpenDash flash image")


env.AddPostAction(app_bin, merge_firmware)

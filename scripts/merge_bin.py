Import("env")

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
BOARD_CONFIG = env.BoardConfig()


def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]

    flash_size = env.BoardConfig().get("upload.flash_size", "4MB")
    flash_freq = env.BoardConfig().get("build.f_flash", "40000000L")
    flash_freq = str(flash_freq).replace("L", "")
    flash_freq = str(int(int(flash_freq) / 1000000)) + "m"
    flash_mode = env.BoardConfig().get("build.flash_mode", "dio")
    memory_type = env.BoardConfig().get("build.arduino.memory_type", "qio_qspi")

    if flash_mode == "qio" or flash_mode == "qout":
        flash_mode = "dio"
    if memory_type == "opi_opi" or memory_type == "opi_qspi":
        flash_mode = "dout"

    # Run esptool to merge images into a single binary
    def merge(output, partitions):
        env.Execute(
            " ".join(
                [
                    "$PYTHONEXE",
                    "$OBJCOPY",
                    "--chip",
                    BOARD_CONFIG.get("build.mcu", "esp32"),
                    "merge_bin",
                    "-o",
                    output,
                    "--flash_mode",
                    flash_mode,
                    "--flash_freq",
                    flash_freq,
                    "--flash_size",
                    flash_size,
                ] + partitions
            )
        )

    merge("$BUILD_DIR/${PROGNAME}_merged_native.bin", [
        "0x1000", "$BUILD_DIR/bootloader.bin",
        "0x8000", "$BUILD_DIR/partitions.bin",
        "0x10000", APP_BIN,
        "0x670000", "$BUILD_DIR/spiffs.bin"
    ])

    merge("$BUILD_DIR/${PROGNAME}_merged_wokwi_simulator.bin", [
        "0x0", "$BUILD_DIR/bootloader.bin",
        "0x8000", "$BUILD_DIR/partitions.bin",
        "0x10000", APP_BIN,
        "0x670000", "$BUILD_DIR/spiffs.bin"
    ])

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN , merge_bin)

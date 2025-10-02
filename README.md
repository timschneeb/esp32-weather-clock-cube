# Desk Weather Clock (GeekMagic-S3)
ESP32-S3 firmware for displaying current time, date, weather, and image notifications from other devices on a 240x240 display.

**Built for the [GeekMagic-S3](https://github.com/GeekMagicClock/GeekMagic-S3) bought from AliExpress** (based on `esp32-s3-devkitm-1` with 16MB flash, 8MB PSRAM and a 240x240 TFT display)

This project is designed to be built using [PlatformIO](https://platformio.org/)

<img src="meta/demo.jpg" width="150" />

---

### Features

- Support for the built-in capacitive touch button on top of the chassis to toggle power
- Can display remote images by calling the `/show_image?url=<image_url>` endpoint
- Synchronizes its power state with your computer (device must call `/keepalive` endpoint at least every 4 minutes to stay on)
- Weather Display using OpenWeatherMap API (2.5), including temperature, humidity, min/max temperature, and icon rendering.
- Clock Display with date, time, and weather info.
- Web Configuration UI: WiFi, MQTT, Frigate IP, weather API key, display settings, and more.
- Persistent Storage using Preferences and SPIFFS to save settings and event images.
- Fallback AP-mode if WiFi is not available.
- Live Frigate Notifications via MQTT on ESP32-S3 with automatic image downloading.
- Image Slideshow of recent events with zone labeling, auto-clearing logic, and memory limits.

### Build
```
pio run -t uploadfs     # Upload SPIFFS flash image
pio run -t upload       # Upload firmware
```

### Wokwi Simulator support

This project can be run in the [Wokwi simulator](https://wokwi.com/). Note that the simulator is much slower than the real hardware, so the display updates will be slow.

To run in Wokwi, build the project using PlatformIO. The build process (specifically `merge_bin.py`) will generate a merged binary image in `.pio/build/esp32-s3-devkitm-1/firmware_merged.bin`.

You can then either manually upload this binary to Wokwi together with the `wokwi.toml` and `diagram.json` files, or use Wokwi VS Code or Jetbrains CLion extension to run the simulator directly from inside your IDE (recommended).

### Acknowledgements

- [GeekMagic-S3-Frigate-Event-Viewer](https://github.com/Marijn0/GeekMagic-S3-Frigate-Event-Viewer) was used as a template and re-written using FreeRTOS and LVGL.

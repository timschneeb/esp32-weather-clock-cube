# Desk Weather Clock (GeekMagic-S3)
ESP32-S3 firmware for displaying current time, date, weather, and image notifications from other devices on a 240x240 display.

**Built for the GeekMagic-S3 bought from AliExpress** (based on `esp32-s3-devkitm-1` with 16MB flash, 8MB PSRAM and a 240x240 TFT display)

This project is designed to be built using [PlatformIO](https://platformio.org/)
---

Features:

- Live Frigate Notifications via MQTT on ESP32-S3 with automatic image downloading.
- Image Slideshow of recent events with zone labeling, auto-clearing logic, and memory limits.
- Weather Display using OpenWeatherMap API (2.5), including temperature, humidity, min/max temperature, and icon rendering.
- Clock Display with date, time, and weather info.
- Brightness Control via schedule or manual setting (PWM-based dimming).
- Web Configuration UI: WiFi, MQTT, Frigate IP, weather API key, display settings, and more.
- Persistent Storage using Preferences and SPIFFS to save settings and event images.
- Fallback AP-mode if WiFi is not available.

### Acknowledgements

- [GeekMagic-S3-Frigate-Event-Viewer](https://github.com/Marijn0/GeekMagic-S3-Frigate-Event-Viewer) was used as a template and re-written using FreeRTOS and LVGL.
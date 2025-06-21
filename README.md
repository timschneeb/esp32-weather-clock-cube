# GeekMagic-S3-Frigate-Event-Viewer
ESP32-S3 firmware for displaying event snapshots from Frigate on a 240x240 display

**Built for:** 
**GeekMagic-S3** (based on `esp32-s3-devkitm-1` with 16MB flash + 240x240 TFT display)

A compact and configurable ESP32-S3 firmware for displaying event snapshots from **Frigate**, weather data, and clock information — all in real time on a 240x240 screen.

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

---

![Image-Clock](https://github.com/user-attachments/assets/594025cd-9397-4365-9d6d-80f1bfe6bb6b)

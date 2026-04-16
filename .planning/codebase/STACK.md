# Technology Stack

## Core Language & Framework
- **Language**: C++ (primary codebase)
- **Framework**: ESP-IDF (Espressif IoT Development Framework)
- **Minimum IDF Version**: 5.5.2
- **Build System**: CMake + ESP-IDF Component Manager

## Target Hardware
- **Chips**: ESP32-S3, ESP32, ESP32-C3, ESP32-C5, ESP32-C6, ESP32-P4
- **Boards**: 97+ pre-configured board types (M5Stack, ESP-BOX-3, Waveshare, etc.)

## Multimedia & UI
- **UI Architecture**: LVGL 9.5.0 (Light and Versatile Graphics Library)
- **Audio Codecs**: Opus (Encoder/Decoder), OGG Demuxer
- **Graphics Components**: `esp_mmap_assets`, `image_player`, `esp_new_jpeg`
- **Fonts & Emojis**: `xiaozhi-fonts`, `otto-emoji-gif-component`

## Network & Communication
- **Protocols**: WebSocket, MQTT + UDP
- **Connectivity**: WiFi, 4G CAT.1 (ML307, EC801E), USB RNDIS, Ethernet (UART Modem)

## Services & System
- **RTOS**: FreeRTOS
- **Storage**: NVS (Non-Volatile Storage) for settings, SPIFFS/Partition for assets
- **OTA**: Custom firmware update mechanism via HTTPS
- **State Machine**: Custom C++ implementation with FreeRTOS EventGroups

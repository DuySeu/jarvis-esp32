# Testing & Quality Assurance

## Hardware-in-the-Loop (HIL) Testing
The project primarily relies on testing directly on ESP32 hardware due to the tight integration with audio codecs, displays, and sensors.
- **Audio Testing State**: The `DeviceStateMachine` includes a `kDeviceStateAudioTesting` state specifically for verifying audio input/output functionality.
- **Acoustic Check**: Scripts in `scripts/acoustic_check/` are used to analyze audio performance and quality (e.g., `demod.py`, `graphic.py`).

## Automated Local Testing
- **Audio Debug Server**: `scripts/audio_debug_server.py` allows for debugging the audio stream interaction with a mock or local server.
- **Asset Validation**: `scripts/build_default_assets.py` includes logic to validate and checksum asset partitions (fonts, images, animations) before deployment.

## Diagnostics & Intelligence
- **System Info**: The `SystemInfo` class provides runtime diagnostics, including memory usage (Heap, PSRAM), chip temperature, and task status.
- **Logging**: Extensive use of `ESP_LOG` at various levels (Info, Debug, Error) for runtime monitoring over UART.

## Debugging Tools
- **Core Dumps**: Configured via SDKCONFIG to capture state on fatal errors.
- **Serial Monitor**: Primary tool for observing application flow and event transitions.
- **LCD Debugging**: Many board configurations include UI elements to show connection status, IP addresses, and error messages directly on the device.

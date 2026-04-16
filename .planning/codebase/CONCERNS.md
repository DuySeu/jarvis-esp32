# Technical Concerns & Tech Debt

## Portability & Fragmentation
- **Board Support**: With 97+ supported boards, maintaining and testing firmware updates across different chip variants (ESP32-S3, C3, P4, etc.) and hardware configurations is a major challenge.
- **Display Diversity**: Supporting a wide range of LCDs, OLEDs, and animation strategies requires complex conditional compilation and abstraction layers.

## Performance & Resources
- **Memory Pressure**: The application uses resource-heavy components like LVGL and Opus codecs. Managing heap and PSRAM usage, especially on smaller chips like ESP32-C3, requires careful monitoring.
- **Task Priorities**: The system manages several high-priority tasks (Audio, Codec). Incorrect priority configuration could lead to audio glitches or UI unresponsiveness.

## Reliability & Connectivity
- **Modem Initialization**: Modem detection and network registration (ML307) have retry loops and timeouts that may need tuning for different cellular environments.
- **Power Management**: Power save levels for certain network boards (e.g., ML307) are still marked as `TODO` and need implementation.
- **Error Handling**: While the state machine handles transitions to `FatalError`, some edge cases in network disconnects or audio buffer overflows might lead to silent failures or watchdog resets.

## Feature Completeness
- **Wake Word Accuracy**: Reliance on cloud-based ASR means local wake word detection needs to be extremely robust to avoid false triggers while maintaining sensitivity.
- **OTA Robustness**: Managing OTA for both firmware and asset partitions adds complexity to the update flow and recovery mechanisms.

## Tech Debt
- **Common Board Logic**: Some methods in `common/` board classes still have placeholders or `TODO` comments regarding power management and specialized peripheral support.
- **Fragmentation in Board Classes**: The large number of board directories leads to duplicated logic that could be further consolidated into shared mixins or components.

# External Integrations

## AI Cloud Services
- **LLM Providers**: Referenced support for Qwen, DeepSeek, and other LLMs (accessed via backend server)
- **Audio Processing**: Cloud-based ASR (Automatic Speech Recognition) and TTS (Text-to-Speech)
- **Protocol**: Custom JSON-based protocol over WebSocket/MQTT for AI interaction

## Device-Side Protocols
- **MCP (Model Context Protocol)**: Device-side server implementation for AI tool calling
- **WebSocket**: Primary bi-directional communication for streaming audio and control
- **MQTT + UDP**: Alternative protocol for low-latency command/control

## Hardware Peripherals
- **LCD/OLED Drivers**: Support for 20+ different display controllers (ST7789, ILI9341, SSD1306, etc.)
- **Audio Hardware**: I2S codecs (ES8311, ES8388, etc.), DACs, and built-in ADC
- **Sensors/Actuators**: Battery estimation (ADC), BMI270 (IMU), Servo control (servo_dog_ctrl)

## Infrastructure Integrations
- **OTA Updates**: HTTPS-based firmware and asset partition updates
- **WiFi Config**: Supports BluFi or similar configuration methods
- **Component Registry**: Heavy usage of the ESP-IDF Component Registry for driver management

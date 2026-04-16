# Directory Structure

## Root Level
- `main/`: Primary source code (C++/ESP-IDF Component)
- `partitions/`: Flash partition tables for different memory layouts
- `scripts/`: Python scripts for asset generation, language processing, and build automation
- `docs/`: Component and protocol documentation
- `CMakeLists.txt`: Root project build configuration

## Primary Source (`main/`)
- `main.cc`: Application entry point (`app_main`)
- `application.{h,cc}`: Core application singleton
- `device_state_machine.{h,cc}`: Central state management
- `mcp_server.{h,cc}`: MCP tool implementation
- `audio/`: Audio pipeline, codecs, and processing logic
- `display/`: Display abstraction and LVGL theme/ui implementation
- `protocols/`: Communication protocol implementations (WebSocket, MQTT)
- `boards/`: Configuration and drivers for 97+ supported hardware boards
- `led/`: LED and light strip control logic
- `assets/`: Resource management and locales

## Data Files
- `idf_component.yml`: Component dependency manifest
- `Kconfig.projbuild`: Project-wide menuconfig options
- `sdkconfig.defaults.*`: Default configurations for different ESP32 chip variants
- `gsd-file-manifest.json`: GSD internal tracking (if installed)

## Key Metrics
- **Files**: ~533 source files
- **LOC**: ~94,134 lines of code
- **Boards**: 97 distinct board configurations
- **Languages**: 30+ localization files

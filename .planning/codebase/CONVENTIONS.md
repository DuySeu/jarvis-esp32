# Coding Conventions

## Language Standards
- **C++**: The project uses modern C++ (Standard: Latest, as per .clang-format).
- **Style Base**: Google C++ Style Guide (with modifications).

## Naming Conventions
- **Classes & Structs**: `PascalCase` (e.g., `Application`, `AudioService`)
- **Methods**: `PascalCase` (e.g., `Initialize`, `Run`, `SetDeviceState`)
- **Enums**: `PascalCase` (e.g., `AecMode`)
- **Enum Constants**: `kPascalCase` (e.g., `kAecOff`, `kAecOnDeviceSide`)
- **Member Variables**: `snake_case_` with trailing underscore (e.g., `event_group_`, `main_tasks_`)
- **Local Variables**: `snake_case` (e.g., `old_state`, `new_state`)
- **Macros & Definitions**: `SCREAMING_SNAKE_CASE` (e.g., `MAIN_EVENT_SCHEDULE`)
- **Files**: `snake_case` (e.g., `audio_service.cc`, `display.h`)

## Formatting (.clang-format)
- **Indentation**: 4 spaces, no tabs.
- **Column Limit**: 100 characters.
- **Braces**: Attached to the control statement/function signature.
- **Pointer Alignment**: Left (`Type* name`).
- **Includes**: Sorted automatically. Priorities:
  1. `<esp_*.h>` and `<driver/*.h>`
  2. Generic system headers `<*.h>`
  3. Other quoted headers `"*.h"`

## Architecture Patterns
- **Singletons**: Use `static Application& GetInstance()` for core services.
- **Event-Driven**: Use FreeRTOS `EventGroups` for inter-task communication.
- **RAII**: Extensively used for resource management (e.g., `std::unique_ptr`, `TaskPriorityReset`).

## Logging
- Uses standard ESP-IDF logging macros (`ESP_LOGI`, `ESP_LOGE`, etc.) with component-specific tags.

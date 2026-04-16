# Architecture Overview

## Design Patterns
- **Singleton**: Core services like `Application`, `Board`, `McpServer`, and `Assets` use the Singleton pattern for global access and resource management.
- **State Machine**: The device behavior is driven by a `DeviceStateMachine` using FreeRTOS EventGroups for synchronization.
- **Strategy Pattern**: The `Assets` system uses `LvglStrategy` or `EmoteStrategy` based on the display hardware.
- **Abstract Factory**: Boards are instantiated via a `create_board()` factory function with a `DECLARE_BOARD()` registration macro.
- **Observer**: State change listeners allow components to react to device state transitions (e.g., Idle -> Listening).

## Core Components
- **Application Class**: The central orchestrator that manages initialization, main event loop, and component coordination.
- **Audio Service**: Manages the audio pipeline, including capture, encoding (Opus), decoding, and playback.
- **Protocol Layer**: Abstracted communication interface supporting both WebSocket and MQTT implementations.
- **Display Subsystem**: Hierarchical abstraction allowing the codebase to support simple OLEDs, complex LCDs with themes, and animation-focused "Emote" displays.
- **MCP Server**: Implements the Model Context Protocol directly on the device, allowing LLMs to "call tools" like toggling LEDs or moving servos.

## Data Flow
### Audio Pipeline
- **Input**: Mic -> AudioProcessor (AEC/NS) -> Opus Encoder -> Send Queue -> Server
- **Output**: Server -> Decode Queue -> Opus Decoder -> Playback Queue -> Speaker

### State Transitions
The system transitions through defined states: `Starting` -> `Idle` -> `Connecting` -> `Listening` -> `Speaking`.
Transitions are triggered by wake words, button presses, or server commands.

## Concurrency Model
The application leverages FreeRTOS tasks for parallel execution:
1. `main`: Application logic and event loop
2. `AudioInputTask`: Sample capture and processing
3. `AudioOutputTask`: Playback management
4. `OpusCodecTask`: Resource-intensive encoding/decoding
5. `ActivationTask`: One-time setup/activation logic

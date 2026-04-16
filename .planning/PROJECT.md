# Jarvis-ESP32 SmartLife MCP Integration

## What This Is

An extension to the XiaoZhi (Jarvis) ESP32 AI chatbot that enables voice-controlled smart home automation via SmartLife/Tuya devices. Users speak natural commands like "turn on the bedroom light" or "schedule the lights off at 11pm" to the ESP32 device, and the AI automatically controls their SmartLife devices through the Tuya Cloud API — no additional server required.

## Core Value

Voice commands spoken to the ESP32 chatbot must reliably control SmartLife devices in real-time. If the light doesn't turn on when you say "turn on the light," nothing else matters.

## Requirements

### Validated

<!-- Existing capabilities from the codebase -->

- ✓ MCP Server framework with tool registration — existing (`mcp_server.h/.cc`)
- ✓ Device state machine and event-driven architecture — existing
- ✓ WebSocket/MQTT protocol for cloud LLM communication — existing
- ✓ NVS-backed settings storage — existing (`settings.h/.cc`)
- ✓ HTTPS client capability via ESP-IDF — existing
- ✓ 97+ board support including ESP32-S3 — existing
- ✓ Audio pipeline (ASR/TTS) for voice interaction — existing

### Active

<!-- New scope for SmartLife MCP integration -->

- [ ] Tuya Cloud API client on ESP32 — OAuth2 auth, HMAC-SHA256 request signing, token refresh
- [ ] `list_devices` MCP tool — discover and list all SmartLife devices from Tuya Cloud
- [ ] `turn_on_light` MCP tool — control light on/off, brightness, and color via voice command
- [ ] `set_schedule` MCP tool — schedule device actions for a future time (e.g., "turn off lights at 11pm")
- [ ] Device discovery on reset/boot — automatically fetch and cache SmartLife device list
- [ ] Credentials configuration — Tuya Client ID, Client Secret stored in NVS settings
- [ ] Extensible device type support — architecture allows adding more device types in future versions

### Out of Scope

- Local LAN control of Tuya devices — adds complexity without significant benefit when cloud API works
- Custom SmartLife app or mobile companion — user already has the SmartLife app
- Hosting a separate bridge/middleware server — all logic runs on the ESP32 directly
- Non-Tuya/SmartLife smart home platforms (HomeKit, Google Home, Alexa) — focus on SmartLife only
- Device pairing/provisioning — devices are already set up in SmartLife app
- Display UI for device management — voice-only control for v1

## Context

- **Existing codebase**: XiaoZhi ESP32 AI Chatbot (v2.2.5), ~94K LOC C++, ESP-IDF ≥ 5.5.2
- **Target hardware**: ESP32-S3 with PSRAM, WiFi connectivity
- **SmartLife/Tuya ecosystem**: User has an active Tuya Developer account at `iot.tuya.com` with API credentials
- **Current MCP tools**: LED control, Speaker volume, GPIO, Servo — all run on-device
- **Tuya Cloud API**: RESTful HTTPS API with HMAC-SHA256 signing, OAuth2 token-based auth
- **User's devices**: Smart lights in a room, controlled via SmartLife app on smartphone
- **Branch**: `mcp-smartlife` — isolated development branch for this feature

## Constraints

- **Memory**: ESP32-S3 has limited heap; Tuya API responses (device lists, JSON) must be parsed efficiently
- **Network**: Tuya API calls add latency; device cache helps avoid repeated API calls
- **Security**: Tuya API credentials must be stored securely in NVS, not hardcoded
- **Compatibility**: Must integrate with existing McpServer framework without breaking current MCP tools
- **ESP-IDF**: Must use ESP-IDF's native HTTPS client and mbedTLS for cryptographic operations

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| ESP32 calls Tuya Cloud API directly | No extra server to host/maintain; ESP32-S3 has enough resources | — Pending |
| Store Tuya credentials in NVS | Consistent with existing Settings pattern; survives firmware updates | — Pending |
| Auto-discover devices on boot/reset | User wants plug-and-play; avoids manual device ID configuration | — Pending |
| Three MCP tools for v1 (list_devices, turn_on_light, set_schedule) | Covers core use case (room lights) while keeping scope manageable | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-16 after initialization*

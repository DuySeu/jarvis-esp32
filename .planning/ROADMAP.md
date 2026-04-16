# Roadmap: Jarvis-ESP32 SmartLife MCP Integration

## Overview

Build voice-controlled SmartLife automation directly on the ESP32-S3 — no bridge server required. The chatbot calls Tuya Cloud API directly using three MCP tools (`list_devices`, `turn_on_light`, `set_schedule`), enabling natural language control of smart home devices in real-time.

## Phases

- [ ] **Phase 1: Tuya Cloud API Client** - Build authenticated HTTPS client with HMAC-SHA256 signing and OAuth2 token management
- [ ] **Phase 2: Device Discovery & MCP Tool Registration** - Auto-discover SmartLife devices on boot and register all three MCP tools
- [ ] **Phase 3: Device Control & Scheduling MCP Tools** - Implement full turn_on_light and set_schedule tools with end-to-end voice control

## Phase Details

### Phase 1: Tuya Cloud API Client
**Goal**: Build a reusable, authenticated Tuya API client module on ESP32 that can sign and execute HTTPS requests to the Tuya Cloud.
**Depends on**: Nothing (first phase)
**Requirements**: TUYA-01, TUYA-02, TUYA-03, TUYA-04, TUYA-05
**Success Criteria** (what must be TRUE):
  1. ESP32 successfully obtains an access_token from Tuya Cloud (`GET /v1.0/token?grant_type=1`) using NVS credentials
  2. All API requests include valid HMAC-SHA256 signed headers (verifiable via Tuya API 200 response)
  3. Token is auto-refreshed before expiry without user intervention
  4. Tuya Client ID, Secret, and region are readable/writable from NVS settings
**Plans**: TBD

Plans:
- [x] 01-01: NVS credentials + SNTP time sync setup
- [x] 01-02: HMAC-SHA256 signing and esp_http_client wrapper (tuya_client)
- [ ] 01-03: OAuth2 token fetch, storage, and auto-refresh (tuya_auth)

### Phase 2: Device Discovery & MCP Tool Registration
**Goal**: Query Tuya Cloud for all user devices on boot, cache the list, and register the three MCP tools with McpServer.
**Depends on**: Phase 1
**Requirements**: DISC-01, DISC-02, DISC-03, MCP-01, MCP-02
**Success Criteria** (what must be TRUE):
  1. On boot, ESP32 automatically fetches all devices from `GET /v1.0/users/{uid}/devices` and caches them
  2. `list_devices` MCP tool returns device names, categories, and online status to the LLM
  3. All three MCP tools (`list_devices`, `turn_on_light`, `set_schedule`) are visible to the LLM in the tool list
  4. If Tuya credentials are absent from NVS, tools are not registered and the system boots normally
**Plans**: TBD

Plans:
- [ ] 02-01: TuyaDeviceManager singleton with device fetch and cache
- [ ] 02-02: MCP tool registration (list_devices full, turn_on_light/set_schedule stubs)

### Phase 3: Device Control & Scheduling MCP Tools
**Goal**: Implement the full `turn_on_light` and `set_schedule` MCP tools so the AI can control lights and schedule actions via voice commands.
**Depends on**: Phase 2
**Requirements**: CTRL-01, CTRL-02, CTRL-03, CTRL-04, SCHED-01, SCHED-02, SCHED-03, SCHED-04, MCP-03
**Success Criteria** (what must be TRUE):
  1. User says "turn on the bedroom light" → light turns on via Tuya Cloud
  2. User says "turn off the light" → light turns off
  3. User says "set lights off at 11pm" → one-time timer created in Tuya Cloud
  4. Device name matching resolves natural language input (e.g., "bedroom light") to correct device ID
  5. Errors (device not found, offline, API failure) return meaningful messages the LLM reads back to user
**Plans**: TBD

Plans:
- [ ] 03-01: turn_on_light tool — device name resolution + POST /v1.0/devices/{id}/commands
- [ ] 03-02: set_schedule tool — time parsing + POST /v2.0/cloud/timer/device/{id}
- [ ] 03-03: Error handling and end-to-end voice test verification

---
*Roadmap created: 2026-04-16*
*Last updated: 2026-04-16 after initial definition*

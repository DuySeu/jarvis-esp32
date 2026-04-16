# Requirements: Jarvis-ESP32 SmartLife MCP Integration

**Defined:** 2026-04-16
**Core Value:** Voice commands spoken to the ESP32 chatbot must reliably control SmartLife devices in real-time.

## v1 Requirements

### Tuya Client (Auth & Communication)

- [ ] **TUYA-01**: ESP32 can obtain an OAuth2 access token from Tuya Cloud API using stored Client ID and Client Secret
- [ ] **TUYA-02**: ESP32 generates valid HMAC-SHA256 signed request headers for all Tuya API calls (using mbedTLS)
- [ ] **TUYA-03**: Access token is automatically refreshed before expiry without user intervention
- [ ] **TUYA-04**: Tuya Client ID, Client Secret, and region (data center URL) are stored in NVS and configurable via settings
- [ ] **TUYA-05**: ESP32 time is synchronized via SNTP before making any Tuya API requests (required for signature timestamp accuracy)

### Device Discovery

- [ ] **DISC-01**: On first boot or manual reset, ESP32 automatically queries Tuya Cloud API to fetch all devices linked to the account
- [ ] **DISC-02**: Discovered device list (ID, name, category, online status) is cached in memory for the session
- [ ] **DISC-03**: `list_devices` MCP tool returns the cached device list to the LLM, including device name and online/offline status

### Device Control

- [ ] **CTRL-01**: `turn_on_light` MCP tool turns a specified light on (`switch_led: true`) via `POST /v1.0/devices/{device_id}/commands`
- [ ] **CTRL-02**: `turn_on_light` MCP tool turns a specified light off (`switch_led: false`)
- [ ] **CTRL-03**: `turn_on_light` MCP tool can match a device by name (e.g. "bedroom light") from the cached device list
- [ ] **CTRL-04**: Successful control commands return a confirmation message to the LLM for voice feedback to the user

### Scheduling

- [ ] **SCHED-01**: `set_schedule` MCP tool creates a one-time timer on Tuya Cloud via `POST /v2.0/cloud/timer/device/{device_id}` for a specified device and time
- [ ] **SCHED-02**: `set_schedule` accepts natural-language time input (e.g. "at 11pm", "in 30 minutes") and converts to Tuya timer format
- [ ] **SCHED-03**: `set_schedule` accepts the device name (resolved from cached list) and desired action (on/off)
- [ ] **SCHED-04**: Successful schedule creation returns a confirmation message to the LLM with the scheduled time

### MCP Integration

- [ ] **MCP-01**: Three MCP tools (`list_devices`, `turn_on_light`, `set_schedule`) are registered with `McpServer` on boot alongside existing tools
- [ ] **MCP-02**: Tools are only registered when Tuya credentials are present in NVS (graceful degradation when unconfigured)
- [ ] **MCP-03**: Tool errors (network failure, device not found, auth failure) return meaningful error strings to the LLM for user feedback

## v2 Requirements

### Extended Device Control

- **CTRL-V2-01**: Set brightness level (0-100%) for dimmable lights
- **CTRL-V2-02**: Set color temperature or RGB color for color-capable lights
- **CTRL-V2-03**: Control non-light device types (switches, plugs, fans) via generic `control_device` MCP tool

### Advanced Scheduling

- **SCHED-V2-01**: Recurring schedules (e.g. "turn off every night at 11pm")
- **SCHED-V2-02**: List and cancel existing schedules via MCP tool

### Reliability

- **REL-V2-01**: Device cache persists across reboots (stored in NVS or SPIFFS)
- **REL-V2-02**: Retry logic for transient Tuya API failures

## Out of Scope

| Feature | Reason |
|---------|--------|
| Local LAN/Tuya local control | Added complexity; cloud API is sufficient and already used by SmartLife app |
| Hosting a bridge/middleware server | User wants serverless; all logic runs on ESP32 directly |
| Device pairing/provisioning | Devices already set up in SmartLife app — out of scope for v1 |
| Non-Tuya platforms (HomeKit, Alexa, Google Home) | Focus on SmartLife ecosystem only |
| Mobile companion UI | Voice-only; SmartLife app handles manual control |
| Display UI for device management | Not needed for voice-controlled use case |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| TUYA-01 | Phase 1 | Pending |
| TUYA-02 | Phase 1 | Pending |
| TUYA-03 | Phase 1 | Pending |
| TUYA-04 | Phase 1 | Pending |
| TUYA-05 | Phase 1 | Pending |
| DISC-01 | Phase 2 | Pending |
| DISC-02 | Phase 2 | Pending |
| DISC-03 | Phase 2 | Pending |
| CTRL-01 | Phase 3 | Pending |
| CTRL-02 | Phase 3 | Pending |
| CTRL-03 | Phase 3 | Pending |
| CTRL-04 | Phase 3 | Pending |
| SCHED-01 | Phase 3 | Pending |
| SCHED-02 | Phase 3 | Pending |
| SCHED-03 | Phase 3 | Pending |
| SCHED-04 | Phase 3 | Pending |
| MCP-01 | Phase 2 | Pending |
| MCP-02 | Phase 2 | Pending |
| MCP-03 | Phase 3 | Pending |

**Coverage:**
- v1 requirements: 19 total
- Mapped to phases: 19
- Unmapped: 0 ✓

---
*Requirements defined: 2026-04-16*
*Last updated: 2026-04-16 after initial definition*

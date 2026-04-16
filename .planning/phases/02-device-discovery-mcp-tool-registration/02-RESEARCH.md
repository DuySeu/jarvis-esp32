# Phase 2 Research: Device Discovery & MCP Tool Registration

## RESEARCH COMPLETE

---

## 1. Tuya Device Discovery API

### Endpoint
```
GET /v1.0/users/{uid}/devices
```
- **Auth**: requires valid `access_token` (business API path → `client_id + access_token + t + nonce + stringToSign`)
- **UID source**: `TuyaAuth::GetInstance().GetUid()` — already populated during Phase 1 token fetch

### Response structure
```json
{
  "success": true,
  "result": [
    {
      "id":           "vdevo123456789xxxxxx",  // device_id for commands
      "name":         "Living Room Light",
      "online":       true,
      "category":     "dj",                    // dj=bulb, kg=switch, etc.
      "product_name": "Smart Bulb"
    }
  ],
  "t": 1714567890123
}
```

**Key fields for our tools:**
| Field | Used in |
|-------|---------|
| `id` | `turn_on_light` device_id, `set_schedule` device_id |
| `name` | `list_devices`, device name lookup |
| `online` | `list_devices` online status |
| `category` | `list_devices` category, type hint |

### Parsing strategy (heap-constrained ESP32)
```cpp
struct TuyaDevice {
    std::string id;
    std::string name;
    bool online;
    std::string category;
};
std::vector<TuyaDevice> devices_;
```
- Parse with `cJSON_Parse()`, iterate `cJSON_GetArraySize()` / `cJSON_GetArrayItem()`
- Delete cJSON root immediately after copying strings to `std::vector`
- Typical room = 5–20 devices; ~1KB per device struct in RAM is fine for ESP32-S3 (8MB PSRAM)

---

## 2. Device Control API

### Endpoint
```
POST /v1.0/devices/{device_id}/commands
```

### Request body (JSON string)
```json
{ "commands": [{ "code": "switch_led", "value": true }] }
```
- `true` = ON, `false` = OFF
- Body SHA256 must be computed over this string (TuyaClient::BuildHeaders already handles this)

### Response (success)
```json
{ "success": true, "result": true }
```

### Name → device_id resolution
The `turn_on_light` MCP tool receives a device **name** from the LLM.
Resolution algorithm:
1. Normalise both sides: `std::transform(..., ::tolower)` + trim
2. Exact match first
3. Substring match fallback (`name.find(query) != std::string::npos`)
4. Return error string if no match (MCP-03)

---

## 3. MCP Tool Registration Pattern

### How `McpServer::AddTool` works (from source)
```cpp
// Signature
void AddTool(const std::string& name,
             const std::string& description,
             const PropertyList& properties,
             std::function<ReturnValue(const PropertyList&)> callback);

// Types
using ReturnValue = std::variant<bool, int, std::string, cJSON*, ImageContent*>;

// Property types
kPropertyTypeBoolean, kPropertyTypeInteger, kPropertyTypeString
// Required:  Property("name", kPropertyTypeString)
// Optional:  Property("name", kPropertyTypeString, "default_val")

// Throwing a std::runtime_error from callback → error reply to LLM
```

### Where to register (from application.cc L96-99)
```cpp
// Inside Application::Initialize(), AFTER WiFi connected, on MAIN_EVENT_NETWORK_CONNECTED
auto& mcp_server = McpServer::GetInstance();
mcp_server.AddCommonTools();     // existing
mcp_server.AddUserOnlyTools();   // existing
// ↓ New: call AddSmartLifeTools() after SNTP + token confirm
```

**Correct hook**: The Tuya tools must be registered **after network is connected** (so SNTP has synced and token can be fetched). The event `MAIN_EVENT_NETWORK_CONNECTED` triggers `HandleNetworkConnectedEvent()` → `ActivationTask()` (in a separate xTask). The cleanest insert point is **end of `HandleActivationDoneEvent()`** (L299), which fires after `ActivationTask` completes — network is up, SNTP sync is done.

### Tool return values
- Success: `return std::string("OK")` or a JSON object string
- Error: `throw std::runtime_error("Device not found: " + name)` — the MCP framework catches this and replies with an error string to the LLM (MCP-03)

---

## 4. Graceful Degradation (MCP-02)

```cpp
if (!TuyaConfig::IsConfigured()) {
    ESP_LOGW(TAG, "Tuya not configured — skipping SmartLife MCP tools");
    return; // don't register any tools
}
```
Check `TuyaConfig::IsConfigured()` before registering tools. System boots normally without them.

---

## 5. TuyaDeviceManager Design

### Singleton pattern (matching TuyaAuth)
```cpp
class TuyaDeviceManager {
public:
    static TuyaDeviceManager& GetInstance() {
        static TuyaDeviceManager instance;
        return instance;
    }
    esp_err_t FetchAndCache();          // call on boot
    std::string FindDeviceId(const std::string& name); // for turn_on_light
    std::string ListDevicesJson();      // for list_devices return value
private:
    std::vector<TuyaDevice> devices_;
};
```

### `ListDevicesJson()` output format (string returned to LLM)
```json
[
  {"name": "Living Room Light", "id": "vdevo123", "online": true, "category": "dj"},
  {"name": "Bedroom Light",     "id": "vdevo456", "online": false, "category": "dj"}
]
```

---

## 6. `set_schedule` Tool — Design Decision

Phase 2 registers `set_schedule` as a **stub** per the roadmap plan note:
> "02-02: MCP tool registration (list_devices full, turn_on_light/set_schedule stubs)"

Stub behavior:
```cpp
AddTool("smartlife.set_schedule", "...",
    PropertyList({ Property("device_name", kPropertyTypeString), Property("action", kPropertyTypeString), Property("time", kPropertyTypeString) }),
    [](const PropertyList& p) -> ReturnValue {
        throw std::runtime_error("set_schedule not yet implemented — coming in Phase 3");
    });
```
This makes the tool visible to the LLM (MCP-01) while Phase 3 implements it (SCHED-01 → SCHED-04).

---

## 7. Memory and Heap Constraints

- ESP32-S3 has 8MB PSRAM — device list strings are small (<1KB total for 20 devices)
- cJSON parsing: always call `cJSON_Delete(root)` to free the parse tree
- `TuyaClient::Get()` response buffer is `std::string` (stack-copied) — freed when response goes out of scope
- No heap_caps_malloc needed for Phase 2

---

## 8. ESP-IDF Task Safety

Tools are called from `Application::Schedule()` (main loop task) — not from ISR context. All `TuyaClient::Get()` / `TuyaClient::Post()` calls are safe on that task.

`FetchAndCache()` must run from a FreeRTOS task (not main/ISR). Best place: end of `HandleActivationDoneEvent()` via `Application::Schedule()`, which runs on the main event loop task with 10 priority.

---

## 9. Files Modified by Phase 2

| File | Change |
|------|--------|
| `main/tuya/tuya_device_manager.h` | New: TuyaDevice struct + TuyaDeviceManager singleton |
| `main/tuya/tuya_device_manager.cc` | New: FetchAndCache, ListDevicesJson, FindDeviceId |
| `main/tuya/tuya_tools.h` | New: RegisterSmartLifeTools() function declaration |
| `main/tuya/tuya_tools.cc` | New: 3 MCP tool registrations |
| `main/application.cc` | Hook: call SNTP sync + RegisterSmartLifeTools() in HandleActivationDoneEvent |
| `main/CMakeLists.txt` | Add tuya_device_manager.cc + tuya_tools.cc to SOURCES |

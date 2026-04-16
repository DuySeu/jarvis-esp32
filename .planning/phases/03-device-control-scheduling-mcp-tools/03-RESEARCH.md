# Phase 3 Research: Device Control & Scheduling MCP Tools

## RESEARCH COMPLETE

---

## 1. Tuya Cloud Commands API (`turn_on_light`)

### Endpoint
```
POST /v1.0/devices/{device_id}/commands
```

### Request Body
```json
{
  "commands": [
    {
      "code": "switch_led",
      "value": true
    }
  ]
}
```

### Device Categories and Codes
Tuya uses standard instruction sets depending on the device category.
- `dj` (Light Bulb): `switch_led`
- `kg` (Switch): `switch_1`
- `cz` (Socket): `switch_1`

To be robust, the `turn_on_light` tool will look at the `TuyaDevice.category` cached in Phase 2.
- If `category == "dj"`, use `"code": "switch_led"`
- Otherwise (or if standard string), use `"code": "switch_1"`

### Memory Constraints
Constructing the JSON payload on ESP32 is simple:
```cpp
std::string payload = "{\"commands\":[{\"code\":\"" + dp_code + "\",\"value\":" + (on ? "true" : "false") + "}]}";
```

---

## 2. Tuya Cloud Timer API (`set_schedule`)

### Endpoint
```
POST /v2.0/cloud/timer/device/{device_id}
```

### Request Body
```json
{
  "alias_name": "Scheduled Task via Jarvis",
  "time": "14:52",
  "timezone_id": "Asia/Shanghai",
  "functions": [
    {
      "code": "switch_led",
      "value": true
    }
  ]
}
```

### Parameters
- `alias_name`: Read-only label for the timer.
- `time`: A 24-hour time format string `"HH:mm"`.
- `timezone_id`: e.g. `"UTC"` or the user's localized timezone. For v1, we can require the user to configure their timezone in Settings if needed, or simply let the LLM map the `"time"` property to a 24-hour string in the user's implicit context. The `set_schedule` tool receives `time`. Since the ESP32 doesn't naturally know localized TZ mapping, we can instruct the LLM to format time specifically: "format time to 24-hour HH:mm string. Assuming local timezone mapping if required." We need to send `"UTC"` or read `timezone_id` from Settings. Let's use standard `"Asia/Shanghai"` or from a setting, but Tuya API accepts `"UTC"`.
Actually, if the timer is Cloud-side, we must provide a tz database string like `"America/Los_Angeles"`. If not known, we can read a config `Settings("tuya", true).GetString("timezone", "UTC")`.

### Natural Language parsing
The MCP tool `smartlife.set_schedule` receives `time` as a `std::string`. The LLM is excellent at parsing natural language, so we should update the MCP tool property `time` to ask the LLM to strictly format it as `"HH:mm"`.
MCP Tool property edit:
`Property("time_hhmm", kPropertyTypeString, "The 24-hour time to schedule the action. Format exactly as 'HH:mm'.")`

---

## 3. Graceful Degradation and Error Handling (MCP-03)

Any failure returned by `TuyaClient::Post()`:
- Missing Token
- 400/500 API responses
- Command not found
Will be thrown as `std::runtime_error("Tuya API Error: ...")`.
The `McpServer::DoToolCall` already catches `std::exception` and formats it as an MCP JSON-RPC error response. The LLM gets the text and speaks it out.

---

## 4. Implementation Plan (Files to modify)

We only need to update the stubs in `main/tuya/tuya_tools.cc`.
No new files are necessary unless we want to neatly separate logic (e.g. `TuyaDeviceManager::SendCommand` and `TuyaDeviceManager::SetSchedule`).
Since we already have `TuyaClient`, we can just instantiate `TuyaClient` directly inside the lambda, or better, add these methods to `TuyaDeviceManager` to encapsulate the Tuya API calls and token logic, then have `tuya_tools.cc` call `TuyaDeviceManager::GetInstance().TurnOn(...)`.

**Updates to `tuya_device_manager.h` / `.cc`:**
```cpp
esp_err_t SendCommand(const std::string& device_id, bool on);
esp_err_t SetSchedule(const std::string& device_id, bool on, const std::string& time_hhmm);
```
These will obtain the token, construct the POST payload, perform the request, and check the Tuya `success: true` response field.

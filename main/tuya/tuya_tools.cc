#include "tuya_tools.h"
#include "tuya_device_manager.h"
#include "tuya_config.h"
#include "mcp_server.h"
#include "esp_log.h"
#include <stdexcept>

static const char* TAG = "TuyaTools";

void RegisterSmartLifeTools() {
    if (!TuyaConfig::IsConfigured()) {
        ESP_LOGW(TAG, "Tuya credentials not found; skipping tool registration");
        return;
    }

    auto& mcp = McpServer::GetInstance();

    // 1. list_devices tool (Fully implemented)
    // DISC-03: Returns the cached device list to the LLM
    mcp.AddTool("smartlife.list_devices",
                "Retrieves the list of SmartLife devices in the user's home.",
                PropertyList(),
                [](const PropertyList& properties) -> ReturnValue {
                    auto& device_mgr = TuyaDeviceManager::GetInstance();
                    if (!device_mgr.HasDevices()) {
                        throw std::runtime_error("No Tuya devices found or failed to fetch device list.");
                    }
                    return device_mgr.ListDevicesJson();
                });

    // 2. turn_on_light tool (Stub for Phase 3)
    mcp.AddTool("smartlife.turn_on_light",
                "Turns a specific smart light or switch on or off. "
                "Always check smartlife.list_devices first to get the exact device name.",
                PropertyList({
                    Property("device_name", kPropertyTypeString),
                    Property("on", kPropertyTypeBoolean)
                }),
                [](const PropertyList& properties) -> ReturnValue {
                    std::string device_name = properties["device_name"].value<std::string>();
                    bool on = properties["on"].value<bool>();
                    
                    std::string device_id = TuyaDeviceManager::GetInstance().FindDeviceId(device_name);
                    if (device_id.empty()) {
                        throw std::runtime_error("Device not found: " + device_name);
                    }

                    esp_err_t err = TuyaDeviceManager::GetInstance().SendCommand(device_id, on);
                    if (err != ESP_OK) {
                        throw std::runtime_error("Failed to execute command on Tuya Cloud.");
                    }

                    return std::string("Successfully turned ") + (on ? "on " : "off ") + device_name;
                });

    // 3. set_schedule tool
    mcp.AddTool("smartlife.set_schedule",
                "Schedules a one-time action for a device to turn on or off at a target time. "
                "Always check smartlife.list_devices first to get the exact device name. ",
                PropertyList({
                    Property("device_name", kPropertyTypeString),
                    Property("on", kPropertyTypeBoolean),
                    Property("time_hhmm", kPropertyTypeString, "The 24-hour time string format 'HH:mm' for this schedule.")
                }),
                [](const PropertyList& properties) -> ReturnValue {
                    std::string device_name = properties["device_name"].value<std::string>();
                    bool on = properties["on"].value<bool>();
                    std::string time_hhmm = properties["time_hhmm"].value<std::string>();

                    // Basic format validation
                    if (time_hhmm.length() != 5 || time_hhmm[2] != ':') {
                        throw std::runtime_error("Invalid time_hhmm format. Must be 'HH:mm' (e.g. '14:30').");
                    }

                    std::string device_id = TuyaDeviceManager::GetInstance().FindDeviceId(device_name);
                    if (device_id.empty()) {
                        throw std::runtime_error("Device not found: " + device_name);
                    }

                    esp_err_t err = TuyaDeviceManager::GetInstance().SetSchedule(device_id, on, time_hhmm);
                    if (err != ESP_OK) {
                        throw std::runtime_error("Failed to set Tuya schedule.");
                    }

                    return std::string("Scheduled ") + device_name + (on ? " ON" : " OFF") + " at " + time_hhmm;
                });

    ESP_LOGI(TAG, "SmartLife MCP tools registered successfully");
}

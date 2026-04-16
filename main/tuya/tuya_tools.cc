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
                    // MCP-03: Error fallback if not found
                    std::string device_name = properties["device_name"].value<std::string>();
                    std::string device_id = TuyaDeviceManager::GetInstance().FindDeviceId(device_name);
                    if (device_id.empty()) {
                        throw std::runtime_error("Device not found: " + device_name);
                    }
                    // Stub for device control phase
                    throw std::runtime_error("turn_on_light not yet implemented (coming in Phase 3)");
                    return false;
                });

    // 3. set_schedule tool (Stub for Phase 3)
    mcp.AddTool("smartlife.set_schedule",
                "Schedules a specific device to turn on or off at a target time. "
                "Time formats supported: 'in X minutes', 'at 11pm', 'in 2 hours', etc.",
                PropertyList({
                    Property("device_name", kPropertyTypeString),
                    Property("action", kPropertyTypeString), // "on" or "off"
                    Property("time", kPropertyTypeString)
                }),
                [](const PropertyList& properties) -> ReturnValue {
                    std::string device_name = properties["device_name"].value<std::string>();
                    std::string device_id = TuyaDeviceManager::GetInstance().FindDeviceId(device_name);
                    if (device_id.empty()) {
                        throw std::runtime_error("Device not found: " + device_name);
                    }
                    throw std::runtime_error("set_schedule not yet implemented (coming in Phase 3)");
                    return false;
                });

    ESP_LOGI(TAG, "SmartLife MCP tools registered successfully");
}

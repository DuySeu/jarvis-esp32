#ifndef TUYA_TOOLS_H
#define TUYA_TOOLS_H

// Registers SmartLife-specific tools (list_devices, turn_on_light, set_schedule).
// Should be called during device activation after SNTP time sync is complete.
// MCP-02: Will safely do nothing if TuyaConfig::IsConfigured() is false.
void RegisterSmartLifeTools();

#endif // TUYA_TOOLS_H

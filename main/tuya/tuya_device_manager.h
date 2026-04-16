#ifndef TUYA_DEVICE_MANAGER_H
#define TUYA_DEVICE_MANAGER_H

#include <string>
#include <vector>
#include "esp_err.h"

// Struct representing a single Tuya-linked SmartLife device.
// Populated from GET /v1.0/users/{uid}/devices response.
struct TuyaDevice {
    std::string id;        // device_id — used in /commands and /timer endpoints
    std::string name;      // user-visible label (e.g. "Living Room Light")
    bool online;           // true if device is currently connected to Tuya Cloud
    std::string category;  // Tuya category code (e.g. "dj"=bulb, "kg"=switch)
};

// TuyaDeviceManager — Singleton managing the in-memory device cache.
// Call FetchAndCache() once after SNTP sync (inside HandleActivationDoneEvent Schedule).
// All other phases call GetInstance() to access the cached list.
class TuyaDeviceManager {
public:
    static TuyaDeviceManager& GetInstance() {
        static TuyaDeviceManager instance;
        return instance;
    }

    // Fetch all devices from GET /v1.0/users/{uid}/devices and cache in devices_.
    // Returns ESP_OK on success, error code on auth or network failure.
    // DISK-01: called on boot/activation (after SNTP sync ensures valid timestamp).
    esp_err_t FetchAndCache();

    // Returns true if device cache is populated (non-empty).
    bool HasDevices() const { return !devices_.empty(); }

    // Returns device count.
    size_t DeviceCount() const { return devices_.size(); }

    // DISC-02: Returns the device_id string for a given device name.
    // Matching: 1) lowercase exact, 2) lowercase substring.
    // Returns empty string if no match found.
    std::string FindDeviceId(const std::string& name) const;

    // DISC-03: Returns a JSON array string of all cached devices.
    // Format: [{"name":"...","id":"...","online":true,"category":"..."},...]
    // Used as the return value of the list_devices MCP tool.
    std::string ListDevicesJson() const;

    // Exposes the raw device list (for future iteration in Phase 3).
    const std::vector<TuyaDevice>& GetDevices() const { return devices_; }

    // Returns the Tuya category code (e.g. "dj" for bulb, "kg" for switch) for a given device_id.
    std::string FindDeviceCategory(const std::string& device_id) const;

    // Sends a turn on/off command to the specified device.
    // Uses 'switch_led' for category 'dj' (lights) and 'switch_1' otherwise.
    esp_err_t SendCommand(const std::string& device_id, bool on);

    // Sends a scheduled timer command to the specified device.
    // 'time_hhmm' must be in "HH:mm" format.
    esp_err_t SetSchedule(const std::string& device_id, bool on, const std::string& time_hhmm);

private:
    TuyaDeviceManager() = default;
    ~TuyaDeviceManager() = default;
    TuyaDeviceManager(const TuyaDeviceManager&) = delete;
    TuyaDeviceManager& operator=(const TuyaDeviceManager&) = delete;

    std::vector<TuyaDevice> devices_;
};

#endif // TUYA_DEVICE_MANAGER_H

#ifndef TUYA_CONFIG_H
#define TUYA_CONFIG_H

#include <string>
#include "settings.h"

// NVS namespace for Tuya credentials
// Keys (all ≤15 chars per ESP-IDF NVS limit):
//   "client_id"     — Tuya Developer client ID
//   "client_secret" — Tuya Developer client secret
//   "region"        — Tuya data center base URL (no trailing slash)
//                     Default: "openapi.tuyaus.com"
//
// SNTP Note: No existing SNTP initialization found in codebase.
// TuyaSntp (tuya_sntp.h) handles SNTP sync before first API call.

class TuyaConfig {
public:
    // Returns stored client_id or empty string if not configured
    static std::string GetClientId() {
        Settings s("tuya", false);
        return s.GetString("client_id", "");
    }

    // Returns stored client_secret or empty string if not configured
    static std::string GetClientSecret() {
        Settings s("tuya", false);
        return s.GetString("client_secret", "");
    }

    // Returns stored region or default US data center
    static std::string GetRegion() {
        Settings s("tuya", false);
        return s.GetString("region", "openapi.tuyaus.com");
    }

    // Returns true only if both client_id and client_secret are non-empty
    static bool IsConfigured() {
        return !GetClientId().empty() && !GetClientSecret().empty();
    }

    // Saves credentials to NVS (call during provisioning / settings UI)
    static void Save(const std::string& client_id,
                     const std::string& client_secret,
                     const std::string& region = "openapi.tuyaus.com") {
        Settings s("tuya", true);
        s.SetString("client_id", client_id);
        s.SetString("client_secret", client_secret);
        s.SetString("region", region);
    }
};

#endif // TUYA_CONFIG_H

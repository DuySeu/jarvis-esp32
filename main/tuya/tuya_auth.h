#ifndef TUYA_AUTH_H
#define TUYA_AUTH_H

#include <string>
#include <ctime>
#include "esp_err.h"

// TuyaAuth — Singleton managing Tuya Cloud OAuth2 token lifecycle.
//
// Token endpoint: GET /v1.0/token?grant_type=1
// Token TTL: 7200 seconds (2 hours)
// Refresh threshold: 80% of TTL = 5760 seconds
//
// Usage:
//   std::string token = TuyaAuth::GetInstance().GetValidToken();
//   if (token.empty()) { /* credentials not configured or network error */ }
//
//   std::string uid = TuyaAuth::GetInstance().GetUid();
//   // uid is available after first successful GetValidToken() call
//   // Used in Phase 2: GET /v1.0/users/{uid}/devices

class TuyaAuth {
public:
    static TuyaAuth& GetInstance() {
        static TuyaAuth instance;
        return instance;
    }

    // Returns a valid access token, fetching/refreshing as needed.
    // Returns empty string if:
    //   - TuyaConfig::IsConfigured() is false (credentials missing)
    //   - Network error during token fetch
    std::string GetValidToken();

    // Returns the user UID extracted from the token response.
    // Available after the first successful GetValidToken() call.
    // Used in Phase 2 for GET /v1.0/users/{uid}/devices
    const std::string& GetUid() const { return uid_; }

    // Force a fresh token fetch (clears cached token, fetches new one)
    esp_err_t Refresh();

private:
    TuyaAuth() = default;
    ~TuyaAuth() = default;
    TuyaAuth(const TuyaAuth&) = delete;
    TuyaAuth& operator=(const TuyaAuth&) = delete;

    // Internal: fetch token from Tuya API and update cached fields.
    esp_err_t FetchToken();

    std::string access_token_;
    std::string uid_;
    time_t token_fetched_at_ = 0;   // epoch seconds when token was fetched
    int token_expire_time_ = 7200;  // TTL in seconds (from API response)

    // Refresh at 80% of TTL (e.g. 5760s out of 7200s default)
    static constexpr float kRefreshThreshold = 0.8f;
};

#endif // TUYA_AUTH_H

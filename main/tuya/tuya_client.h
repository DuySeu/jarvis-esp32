#ifndef TUYA_CLIENT_H
#define TUYA_CLIENT_H

#include <string>
#include <map>
#include "esp_http_client.h"
#include "esp_err.h"
#include "cJSON.h"

// TuyaClient — low-level HTTPS wrapper for the Tuya Cloud OpenAPI.
// Handles Tuya-specific request signing (HMAC-SHA256) per:
// https://developer.tuya.com/en/docs/iot/sign-requests
//
// Two signing paths:
//   Token API (access_token empty):   sign_str = client_id + t + nonce + stringToSign
//   Business API (access_token set):  sign_str = client_id + access_token + t + nonce + stringToSign
//
// Usage:
//   TuyaClient client;
//   std::string body;
//   // Token fetch (no access_token):
//   esp_err_t err = client.Get("/v1.0/token?grant_type=1", "", body);
//   // Authenticated call:
//   esp_err_t err = client.Get("/v1.0/users/{uid}/devices", token, body);

class TuyaClient {
public:
    TuyaClient();
    ~TuyaClient();

    // Execute authenticated GET request.
    // path: e.g., "/v1.0/token?grant_type=1"
    // access_token: empty for token-API calls, otherwise the obtained token
    // response_body: output — populated with raw JSON response string
    // Returns ESP_OK on HTTP success (regardless of Tuya error codes in body)
    esp_err_t Get(const std::string& path,
                  const std::string& access_token,
                  std::string& response_body);

    // Execute authenticated POST request with JSON body.
    // post_body: JSON string (e.g., "{\"commands\":[...]}")
    // Returns ESP_OK on HTTP success
    esp_err_t Post(const std::string& path,
                   const std::string& access_token,
                   const std::string& post_body,
                   std::string& response_body);

private:
    // Build Tuya-signed headers map for a request.
    std::map<std::string, std::string> BuildHeaders(
        const std::string& method,
        const std::string& path,
        const std::string& access_token,
        const std::string& body);

    // Compute SHA256 of data, return lowercase hex string (64 chars)
    std::string Sha256Hex(const std::string& data);

    // Compute HMAC-SHA256(key, data), return UPPERCASE hex string (64 chars)
    std::string HmacSha256Upper(const std::string& key, const std::string& data);

    // Convert binary bytes to lowercase hex string
    std::string ToHex(const uint8_t* data, size_t len);

    // Execute HTTP request (internal)
    esp_err_t Execute(const std::string& method,
                      const std::string& url,
                      const std::map<std::string, std::string>& headers,
                      const std::string& body,
                      std::string& response_body,
                      int& status_code);
};

#endif // TUYA_CLIENT_H

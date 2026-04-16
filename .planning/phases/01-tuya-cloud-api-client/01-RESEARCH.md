# Phase 1 Research: Tuya Cloud API Client

## Summary

Research confirms this phase is well-scoped and achievable using existing ESP-IDF primitives. All required cryptographic, HTTP, and storage capabilities are already in the ESP32-S3 toolchain.

---

## 1. Tuya API Authentication Flow

### Token Acquisition (used in tuya_auth.h/.cc)
```
GET https://openapi.tuyaus.com/v1.0/token?grant_type=1

Required Headers:
  client_id: <CLIENT_ID>
  t: <13-digit UTC timestamp in ms>
  nonce: <UUID or empty>
  sign_method: HMAC-SHA256
  sign: HMAC-SHA256(CLIENT_ID + t + nonce + stringToSign, CLIENT_SECRET).toUpperCase()

stringToSign (for token endpoint, no access_token):
  HTTPMethod + "\n" + SHA256(body) + "\n" + "" + "\n" + "/v1.0/token?grant_type=1"
```

### Two Signing Variants
- **Token API**: `str = client_id + t + nonce + stringToSign`
- **Business API** (all other calls): `str = client_id + access_token + t + nonce + stringToSign`

### Token Response
```json
{
  "result": {
    "access_token": "...",
    "refresh_token": "...",
    "expire_time": 7200,
    "uid": "ay15...",
    "platform_url": "..."
  }
}
```
- `uid` is extracted here and used later in `GET /v1.0/users/{uid}/devices`
- Token TTL = 7200s. Refresh at 80% (5760s) using `/v1.0/token/{refresh_token}`

---

## 2. HMAC-SHA256 with mbedTLS (already in ESP-IDF)

```c
#include "mbedtls/md.h"

// output must be 32 bytes
void hmac_sha256(const uint8_t* key, size_t key_len,
                 const uint8_t* data, size_t data_len,
                 uint8_t* output) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1); // 1 = HMAC mode
    mbedtls_md_hmac_starts(&ctx, key, key_len);
    mbedtls_md_hmac_update(&ctx, data, data_len);
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);
}
```

SHA256 of body (for Content-SHA256 header field):
```c
mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), body, body_len, hash_out);
```

Empty body SHA256 (constant): `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`

Result must be hex-encoded and uppercased before placing in headers.

---

## 3. HTTPS with esp_http_client

```cpp
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

esp_http_client_config_t config = {
    .url = "https://openapi.tuyaus.com/v1.0/token?grant_type=1",
    .crt_bundle_attach = esp_crt_bundle_attach, // CA bundle for HTTPS verification
    .timeout_ms = 10000,
};
esp_http_client_handle_t client = esp_http_client_init(&config);

// Set headers
esp_http_client_set_header(client, "client_id", client_id.c_str());
esp_http_client_set_header(client, "sign", sign.c_str());
esp_http_client_set_header(client, "t", timestamp.c_str());
esp_http_client_set_header(client, "sign_method", "HMAC-SHA256");
esp_http_client_set_header(client, "Content-Type", "application/json");

// Capture response via event handler
esp_err_t err = esp_http_client_perform(client);
int status = esp_http_client_get_status_code(client);
esp_http_client_cleanup(client);
```

**Key detail**: Keep the client handle alive across multiple calls to the same host — avoids redundant TLS handshakes.

Response body capture: Use `HTTP_EVENT_ON_DATA` event handler, appending to a `std::string` buffer.

---

## 4. NVS Credentials — aligns with existing Settings class

The `Settings` class (`main/settings.h/.cc`) is already the project-standard NVS wrapper:
```cpp
// Read credentials
Settings settings("tuya", false);
std::string client_id = settings.GetString("client_id", "");
std::string client_secret = settings.GetString("client_secret", "");
std::string region = settings.GetString("region", "openapi.tuyaus.com");

// Write credentials (during setup/provisioning)
Settings settings("tuya", true);
settings.SetString("client_id", "abc123");
settings.SetString("client_secret", "secret");
settings.SetString("region", "openapi.tuyaeu.com");
```

NVS namespace: `"tuya"` (15 char max enforced by ESP-IDF)
Keys: `"client_id"`, `"client_secret"`, `"region"` (all ≤ 15 chars)

---

## 5. SNTP Time Sync

The Tuya signature requires a 13-digit UTC timestamp (milliseconds). The ESP32 must sync via NTP first.

```cpp
#include "esp_sntp.h"

void sync_time() {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    // Wait for sync
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int64_t get_timestamp_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}
```

SNTP is already used by the codebase for time-sensitive features — check `main/` for existing `esp_sntp_init` calls to avoid duplicate initialization.

---

## 6. Proposed Module Structure

```
main/tuya/
├── tuya_client.h/.cc     — HTTP wrapper: sign + execute Tuya API requests
├── tuya_auth.h/.cc       — Token management: fetch, store (in Settings), refresh
└── CMakeLists.txt        — Add to main component SRCS
```

The `main/CMakeLists.txt` includes sources via `GLOB` or explicit listing — tuya files go in `main/tuya/` and are included via the parent CMakeLists.

---

## 7. Pitfalls to Avoid

| Pitfall | Prevention |
|---------|-----------|
| Timestamp drift → 403 errors | Always sync SNTP before first sign. Re-sync periodically. |
| Region mismatch | Store region in NVS; default to `openapi.tuyaus.com` as fallback |
| TLS handshake timeout | Use `esp_crt_bundle_attach`; add 10s timeout |
| Token expiry mid-session | Check expiry before every API call; refresh if within 20% of TTL |
| Empty body SHA256 | Use the constant: `e3b0c44...b7852b855` — do not skip for GET |
| Case sensitivity of sign | Always `.toUpperCase()` the hex-encoded HMAC result |
| Heap fragmentation from large strings | Build strings in fixed-size char arrays where possible |

---

## RESEARCH COMPLETE

Phase 1 is ready to plan. All technical primitives (mbedTLS, esp_http_client, Settings/NVS, SNTP) are available in the existing codebase. No new dependencies needed.

#include "tuya_auth.h"
#include "tuya_client.h"
#include "tuya_config.h"
#include "esp_log.h"
#include "cJSON.h"
#include <ctime>

static const char* TAG = "TuyaAuth";

// Token response structure from GET /v1.0/token?grant_type=1:
// {
//   "success": true,
//   "result": {
//     "access_token": "...",
//     "expire_time": 7200,
//     "uid": "ay15...",
//     "refresh_token": "..."
//   }
// }

esp_err_t TuyaAuth::FetchToken() {
    if (!TuyaConfig::IsConfigured()) {
        ESP_LOGW(TAG, "Tuya credentials not configured in NVS — skipping token fetch");
        return ESP_ERR_INVALID_STATE;
    }

    TuyaClient client;
    std::string response;
    esp_err_t err = client.Get("/v1.0/token?grant_type=1", "", response);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Token request failed: %s", esp_err_to_name(err));
        return err;
    }

    if (response.empty()) {
        ESP_LOGE(TAG, "Token response is empty");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Parse JSON response
    cJSON* root = cJSON_Parse(response.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse token response JSON: %s",
                 response.substr(0, 200).c_str());
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Check "success" field
    cJSON* success = cJSON_GetObjectItem(root, "success");
    if (!cJSON_IsTrue(success)) {
        cJSON* code = cJSON_GetObjectItem(root, "code");
        cJSON* msg = cJSON_GetObjectItem(root, "msg");
        ESP_LOGE(TAG, "Token API error — code: %d, msg: %s",
                 (code && cJSON_IsNumber(code)) ? code->valueint : 0,
                 (msg && cJSON_IsString(msg)) ? msg->valuestring : "unknown");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // Extract result fields
    cJSON* result = cJSON_GetObjectItem(root, "result");
    if (!result) {
        ESP_LOGE(TAG, "Token response missing 'result' field");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON* token_json = cJSON_GetObjectItem(result, "access_token");
    cJSON* expire_json = cJSON_GetObjectItem(result, "expire_time");
    cJSON* uid_json = cJSON_GetObjectItem(result, "uid");

    if (!token_json || !cJSON_IsString(token_json)) {
        ESP_LOGE(TAG, "Token response missing 'access_token'");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Commit fetched values
    access_token_ = token_json->valuestring;
    token_fetched_at_ = time(nullptr);

    if (expire_json && cJSON_IsNumber(expire_json)) {
        token_expire_time_ = expire_json->valueint;
    }

    if (uid_json && cJSON_IsString(uid_json)) {
        uid_ = uid_json->valuestring;
        ESP_LOGI(TAG, "Tuya UID: %s", uid_.c_str());
    } else {
        ESP_LOGW(TAG, "Token response missing 'uid' field");
    }

    ESP_LOGI(TAG, "Access token obtained (TTL=%ds)", token_expire_time_);
    cJSON_Delete(root);
    return ESP_OK;
}

std::string TuyaAuth::GetValidToken() {
    // Guard: no credentials means no token
    if (!TuyaConfig::IsConfigured()) {
        return "";
    }

    time_t now = time(nullptr);
    bool token_empty = access_token_.empty();
    // Refresh when elapsed time exceeds kRefreshThreshold × TTL
    bool token_near_expiry = !token_empty &&
        ((now - token_fetched_at_) >= static_cast<time_t>(token_expire_time_ * kRefreshThreshold));

    if (token_empty || token_near_expiry) {
        ESP_LOGI(TAG, "%s — fetching new Tuya token",
                 token_empty ? "No token cached" : "Token near expiry");
        if (FetchToken() != ESP_OK) {
            return "";
        }
    }

    return access_token_;
}

esp_err_t TuyaAuth::Refresh() {
    access_token_.clear();
    token_fetched_at_ = 0;
    return FetchToken();
}

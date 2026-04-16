#include "tuya_device_manager.h"
#include "tuya_auth.h"
#include "tuya_client.h"
#include "tuya_config.h"
#include "esp_log.h"
#include "cJSON.h"
#include <algorithm>
#include <sstream>

static const char* TAG = "TuyaDeviceManager";

esp_err_t TuyaDeviceManager::FetchAndCache() {
    if (!TuyaConfig::IsConfigured()) {
        ESP_LOGW(TAG, "Tuya not configured — skipping device fetch");
        return ESP_ERR_INVALID_STATE;
    }

    // Obtain valid OAuth2 token
    std::string token = TuyaAuth::GetInstance().GetValidToken();
    if (token.empty()) {
        ESP_LOGE(TAG, "No valid Tuya token — cannot fetch devices");
        return ESP_FAIL;
    }

    // Obtain UID (available after first successful GetValidToken())
    std::string uid = TuyaAuth::GetInstance().GetUid();
    if (uid.empty()) {
        ESP_LOGE(TAG, "Tuya UID not available — cannot fetch devices");
        return ESP_FAIL;
    }

    // Build endpoint
    std::string path = "/v1.0/users/" + uid + "/devices";

    TuyaClient client;
    std::string response;
    esp_err_t err = client.Get(path, token, response);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch device list: %s", esp_err_to_name(err));
        return err;
    }

    if (response.empty()) {
        ESP_LOGE(TAG, "Empty device list response");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Parse JSON
    cJSON* root = cJSON_Parse(response.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse device list JSON: %s", response.substr(0, 200).c_str());
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON* success = cJSON_GetObjectItem(root, "success");
    if (!cJSON_IsTrue(success)) {
        cJSON* code = cJSON_GetObjectItem(root, "code");
        cJSON* msg  = cJSON_GetObjectItem(root, "msg");
        ESP_LOGE(TAG, "Device list API error — code: %d, msg: %s",
                 (code && cJSON_IsNumber(code)) ? code->valueint : 0,
                 (msg  && cJSON_IsString(msg))  ? msg->valuestring : "unknown");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON* result = cJSON_GetObjectItem(root, "result");
    if (!cJSON_IsArray(result)) {
        ESP_LOGE(TAG, "Device list 'result' is not an array");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Clear previous cache and populate
    devices_.clear();
    int count = cJSON_GetArraySize(result);
    for (int i = 0; i < count; i++) {
        cJSON* item = cJSON_GetArrayItem(result, i);
        if (!cJSON_IsObject(item)) continue;

        TuyaDevice dev;
        cJSON* id_j       = cJSON_GetObjectItem(item, "id");
        cJSON* name_j     = cJSON_GetObjectItem(item, "name");
        cJSON* online_j   = cJSON_GetObjectItem(item, "online");
        cJSON* category_j = cJSON_GetObjectItem(item, "category");

        if (id_j   && cJSON_IsString(id_j))   dev.id       = id_j->valuestring;
        if (name_j && cJSON_IsString(name_j)) dev.name     = name_j->valuestring;
        dev.online = cJSON_IsTrue(online_j);
        if (category_j && cJSON_IsString(category_j)) dev.category = category_j->valuestring;

        if (!dev.id.empty() && !dev.name.empty()) {
            devices_.push_back(std::move(dev));
        }
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Cached %zu device(s) from Tuya Cloud", devices_.size());
    return ESP_OK;
}

std::string TuyaDeviceManager::FindDeviceId(const std::string& name) const {
    // Normalize query: lowercase
    std::string query = name;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    // 1. Exact match (case-insensitive)
    for (const auto& dev : devices_) {
        std::string dev_lower = dev.name;
        std::transform(dev_lower.begin(), dev_lower.end(), dev_lower.begin(), ::tolower);
        if (dev_lower == query) return dev.id;
    }

    // 2. Substring match fallback
    for (const auto& dev : devices_) {
        std::string dev_lower = dev.name;
        std::transform(dev_lower.begin(), dev_lower.end(), dev_lower.begin(), ::tolower);
        if (dev_lower.find(query) != std::string::npos) return dev.id;
    }

    return ""; // not found
}

std::string TuyaDeviceManager::ListDevicesJson() const {
    // Build JSON array string manually for efficiency (avoid cJSON heap alloc for read-only)
    std::string json = "[";
    for (size_t i = 0; i < devices_.size(); i++) {
        const auto& dev = devices_[i];
        if (i > 0) json += ",";
        json += "{\"name\":\"" + dev.name + "\","
                "\"id\":\""    + dev.id   + "\","
                "\"online\":"  + (dev.online ? "true" : "false") + ","
                "\"category\":\"" + dev.category + "\"}";
    }
    json += "]";
    return json;
}

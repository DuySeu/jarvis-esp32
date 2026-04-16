#include "tuya_device_manager.h"
#include "tuya_auth.h"
#include "tuya_client.h"
#include "tuya_config.h"
#include "settings.h"
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

std::string TuyaDeviceManager::FindDeviceCategory(const std::string& device_id) const {
    for (const auto& dev : devices_) {
        if (dev.id == device_id) {
            return dev.category;
        }
    }
    return "";
}

esp_err_t TuyaDeviceManager::SendCommand(const std::string& device_id, bool on) {
    std::string token = TuyaAuth::GetInstance().GetValidToken();
    if (token.empty()) {
        ESP_LOGE(TAG, "No valid token for SendCommand");
        return ESP_FAIL;
    }

    std::string category = FindDeviceCategory(device_id);
    std::string code = (category == "dj") ? "switch_led" : "switch_1";

    std::string path = "/v1.0/devices/" + device_id + "/commands";
    std::string payload = "{\"commands\":[{\"code\":\"" + code + "\",\"value\":" + (on ? "true" : "false") + "}]}";

    TuyaClient client;
    std::string response;
    esp_err_t err = client.Post(path, token, payload, response);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SendCommand Post failed: %s", esp_err_to_name(err));
        return err;
    }

    // Parse JSON to check Tuya success flag
    cJSON* root = cJSON_Parse(response.c_str());
    if (!root) {
        ESP_LOGE(TAG, "SendCommand invalid JSON");
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON* success = cJSON_GetObjectItem(root, "success");
    if (!cJSON_IsTrue(success)) {
        cJSON* code_j = cJSON_GetObjectItem(root, "code");
        cJSON* msg_j  = cJSON_GetObjectItem(root, "msg");
        ESP_LOGE(TAG, "Tuya Command Error: %d - %s", 
                 (code_j && cJSON_IsNumber(code_j)) ? code_j->valueint : 0,
                 (msg_j && cJSON_IsString(msg_j)) ? msg_j->valuestring : "unknown");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t TuyaDeviceManager::SetSchedule(const std::string& device_id, bool on, const std::string& time_hhmm) {
    std::string token = TuyaAuth::GetInstance().GetValidToken();
    if (token.empty()) {
        ESP_LOGE(TAG, "No valid token for SetSchedule");
        return ESP_FAIL;
    }

    std::string category = FindDeviceCategory(device_id);
    std::string code = (category == "dj") ? "switch_led" : "switch_1";

    std::string path = "/v2.0/cloud/timer/device/" + device_id;
    
    // Read optional timezone from NVS settings, default to UTC if missing.
    Settings settings("tuya", true);
    std::string timezone = settings.GetString("timezone", "UTC");

    // Compose payload
    std::string payload = "{"
        "\"alias_name\":\"Jarvis Timer\","
        "\"time\":\"" + time_hhmm + "\","
        "\"timezone_id\":\"" + timezone + "\","
        "\"functions\":[{\"code\":\"" + code + "\",\"value\":" + (on ? "true" : "false") + "}]"
    "}";

    TuyaClient client;
    std::string response;
    esp_err_t err = client.Post(path, token, payload, response);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SetSchedule Post failed: %s", esp_err_to_name(err));
        return err;
    }

    // Parse JSON
    cJSON* root = cJSON_Parse(response.c_str());
    if (!root) {
        ESP_LOGE(TAG, "SetSchedule invalid JSON");
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON* success = cJSON_GetObjectItem(root, "success");
    if (!cJSON_IsTrue(success)) {
        cJSON* code_j = cJSON_GetObjectItem(root, "code");
        cJSON* msg_j  = cJSON_GetObjectItem(root, "msg");
        ESP_LOGE(TAG, "Tuya Timer Error: %d - %s", 
                 (code_j && cJSON_IsNumber(code_j)) ? code_j->valueint : 0,
                 (msg_j && cJSON_IsString(msg_j)) ? msg_j->valuestring : "unknown");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON_Delete(root);
    return ESP_OK;
}

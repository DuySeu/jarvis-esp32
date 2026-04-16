#include "tuya_client.h"
#include "tuya_config.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include <sys/time.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

static const char* TAG = "TuyaClient";

// =====================================================================
// Response capture event handler
// =====================================================================

struct ResponseBuffer {
    std::string* body;
};

static esp_err_t HttpEventHandler(esp_http_client_event_t* evt) {
    auto* rb = static_cast<ResponseBuffer*>(evt->user_data);
    if (evt->event_id == HTTP_EVENT_ON_DATA && rb && rb->body) {
        rb->body->append(static_cast<const char*>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

// =====================================================================
// TuyaClient implementation
// =====================================================================

TuyaClient::TuyaClient() {}
TuyaClient::~TuyaClient() {}

// ---- Crypto helpers ----

std::string TuyaClient::ToHex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string TuyaClient::Sha256Hex(const std::string& data) {
    uint8_t hash[32];
    mbedtls_sha256(
        reinterpret_cast<const uint8_t*>(data.data()),
        data.size(),
        hash,
        0 /* is224=0 → SHA-256 */
    );
    return ToHex(hash, 32);
}

std::string TuyaClient::HmacSha256Upper(const std::string& key, const std::string& data) {
    uint8_t output[32];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1); // 1 = enable HMAC mode
    mbedtls_md_hmac_starts(&ctx,
        reinterpret_cast<const uint8_t*>(key.data()), key.size());
    mbedtls_md_hmac_update(&ctx,
        reinterpret_cast<const uint8_t*>(data.data()), data.size());
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);

    std::string hex = ToHex(output, 32);
    // Tuya requires uppercase HMAC-SHA256 hex
    std::transform(hex.begin(), hex.end(), hex.begin(), ::toupper);
    return hex;
}

// ---- Header building ----

std::map<std::string, std::string> TuyaClient::BuildHeaders(
        const std::string& method,
        const std::string& path,
        const std::string& access_token,
        const std::string& body) {

    std::string client_id = TuyaConfig::GetClientId();
    std::string client_secret = TuyaConfig::GetClientSecret();

    // Get 13-digit UTC timestamp in milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t timestamp_ms = static_cast<int64_t>(tv.tv_sec) * 1000LL + tv.tv_usec / 1000;
    std::string t = std::to_string(timestamp_ms);
    std::string nonce = ""; // optional UUID — empty is valid per Tuya spec

    // Build stringToSign:
    // HTTPMethod + "\n" + SHA256(body) + "\n" + "" (headers) + "\n" + path
    // Empty body SHA256 is a well-known constant
    std::string body_sha256 = body.empty()
        ? "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        : Sha256Hex(body);
    std::string string_to_sign = method + "\n" + body_sha256 + "\n" + "" + "\n" + path;

    // Two signing paths per Tuya spec:
    // Token API (no access_token): client_id + t + nonce + stringToSign
    // Business API (with access_token): client_id + access_token + t + nonce + stringToSign
    std::string sign_str;
    if (access_token.empty()) {
        sign_str = client_id + t + nonce + string_to_sign;
    } else {
        sign_str = client_id + access_token + t + nonce + string_to_sign;
    }

    std::string sign = HmacSha256Upper(client_secret, sign_str);

    std::map<std::string, std::string> headers;
    headers["client_id"] = client_id;
    headers["t"] = t;
    headers["sign_method"] = "HMAC-SHA256";
    headers["sign"] = sign;
    headers["Content-Type"] = "application/json";
    if (!access_token.empty()) {
        headers["access_token"] = access_token;
    }
    return headers;
}

// ---- HTTP execution ----

esp_err_t TuyaClient::Execute(
        const std::string& method,
        const std::string& url,
        const std::map<std::string, std::string>& headers,
        const std::string& body,
        std::string& response_body,
        int& status_code) {

    response_body.clear();
    status_code = 0;
    ResponseBuffer rb{&response_body};

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.event_handler = HttpEventHandler;
    config.user_data = &rb;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 10000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for: %s", url.c_str());
        return ESP_FAIL;
    }

    // Set HTTP method
    if (method == "POST") {
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        if (!body.empty()) {
            esp_http_client_set_post_field(client, body.c_str(),
                                           static_cast<int>(body.size()));
        }
    } else {
        esp_http_client_set_method(client, HTTP_METHOD_GET);
    }

    // Set all signed Tuya headers
    for (const auto& [key, val] : headers) {
        esp_http_client_set_header(client, key.c_str(), val.c_str());
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "%s %s → HTTP %d (%zu bytes)",
                 method.c_str(), url.c_str(), status_code, response_body.size());
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t TuyaClient::Get(const std::string& path,
                           const std::string& access_token,
                           std::string& response_body) {
    std::string region = TuyaConfig::GetRegion();
    std::string url = "https://" + region + path;
    auto headers = BuildHeaders("GET", path, access_token, "");
    int status_code = 0;
    return Execute("GET", url, headers, "", response_body, status_code);
}

esp_err_t TuyaClient::Post(const std::string& path,
                            const std::string& access_token,
                            const std::string& post_body,
                            std::string& response_body) {
    std::string region = TuyaConfig::GetRegion();
    std::string url = "https://" + region + path;
    auto headers = BuildHeaders("POST", path, access_token, post_body);
    int status_code = 0;
    return Execute("POST", url, headers, post_body, response_body, status_code);
}

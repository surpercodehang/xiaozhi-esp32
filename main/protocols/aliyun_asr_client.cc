#include "aliyun_asr_client.h"
#include "board.h"
#include "settings.h"
#include <esp_log.h>
#include <cJSON.h>
#include <mbedtls/base64.h>

#define TAG "AliyunAsrClient"

AliyunAsrClient::AliyunAsrClient() {
}

AliyunAsrClient::~AliyunAsrClient() {
}

bool AliyunAsrClient::Initialize(const std::string& api_key) {
    if (!LoadConfig()) {
        config_.api_key = api_key;
    }

    if (config_.api_key.empty()) {
        ESP_LOGE(TAG, "API Key is not configured");
        return false;
    }

    // 设置默认配置
    if (config_.endpoint.empty()) {
        config_.endpoint = "https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/asr";
    }
    config_.format = "opus";
    config_.sample_rate = 16000;

    ESP_LOGI(TAG, "ASR Client initialized");
    return true;
}

bool AliyunAsrClient::LoadConfig() {
    Settings settings("bailian", false);
    config_.api_key = settings.GetString("api_key");
    return !config_.api_key.empty();
}

std::string AliyunAsrClient::BuildRequestBody(const std::vector<uint8_t>& audio_data) {
    // 将音频数据转为 Base64
    size_t olen = 0;
    mbedtls_base64_encode(nullptr, 0, &olen, audio_data.data(), audio_data.size());
    
    std::vector<uint8_t> base64_buffer(olen);
    mbedtls_base64_encode(base64_buffer.data(), base64_buffer.size(), &olen,
                          audio_data.data(), audio_data.size());
    
    std::string audio_base64(reinterpret_cast<const char*>(base64_buffer.data()), olen);

    // 构造请求 JSON
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "format", config_.format.c_str());
    cJSON_AddNumberToObject(root, "sample_rate", config_.sample_rate);
    cJSON_AddStringToObject(root, "audio", audio_base64.c_str());
    cJSON_AddBoolToObject(root, "enable_punctuation_prediction", config_.enable_punctuation);
    cJSON_AddBoolToObject(root, "enable_inverse_text_normalization", config_.enable_itn);

    char* json_str = cJSON_PrintUnformatted(root);
    std::string body(json_str);
    free(json_str);
    cJSON_Delete(root);

    return body;
}

bool AliyunAsrClient::RecognizeOnce(const std::vector<uint8_t>& audio_data,
                                    OnResultCallback on_result,
                                    OnErrorCallback on_error) {
    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();
    http_ = network->CreateHttp(0);

    if (!http_) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        if (on_error) {
            on_error("Failed to create HTTP client");
        }
        return false;
    }

    // 设置请求头
    std::string auth_header = "Bearer " + config_.api_key;
    http_->SetHeader("Authorization", auth_header.c_str());
    http_->SetHeader("Content-Type", "application/json");

    // 构造请求体
    std::string body = BuildRequestBody(audio_data);
    
    ESP_LOGI(TAG, "Sending ASR request, audio size: %d bytes", audio_data.size());
    
    // 发送请求
    http_->SetContent(std::move(body));
    
    if (!http_->Open("POST", config_.endpoint)) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        if (on_error) {
            on_error("Failed to open HTTP connection");
        }
        return false;
    }

    // 获取响应
    int status_code = http_->GetStatusCode();
    if (status_code != 200) {
        std::string error = "HTTP error: " + std::to_string(status_code);
        ESP_LOGE(TAG, "%s", error.c_str());
        if (on_error) {
            on_error(error);
        }
        return false;
    }

    std::string response_body = http_->ReadAll();
    
    // 解析响应
    if (!ParseResponse(response_body, on_result)) {
        if (on_error) {
            on_error("Failed to parse ASR response");
        }
        return false;
    }

    return true;
}

bool AliyunAsrClient::ParseResponse(const std::string& json_str, OnResultCallback callback) {
    cJSON* json = cJSON_Parse(json_str.c_str());
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return false;
    }

    bool success = false;
    
    // 检查状态
    cJSON* status = cJSON_GetObjectItem(json, "status");
    cJSON* result = cJSON_GetObjectItem(json, "result");
    
    if (cJSON_IsNumber(status) && status->valueint == 20000000) {
        // 成功
        if (cJSON_IsString(result)) {
            Response resp;
            resp.text = result->valuestring;
            resp.is_final = true;
            resp.status = status->valueint;
            
            ESP_LOGI(TAG, "ASR result: %s", resp.text.c_str());
            
            if (callback) {
                callback(resp);
            }
            success = true;
        }
    } else {
        ESP_LOGE(TAG, "ASR failed, status: %d", status ? status->valueint : -1);
    }

    cJSON_Delete(json);
    return success;
}

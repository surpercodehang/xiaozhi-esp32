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

    // 设置默认配置 - 使用一句话识别 API
    if (config_.endpoint.empty()) {
        config_.endpoint = "https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/FlashRecognizer";
    }
    config_.format = "pcm";  // 阿里云 ASR 需要 PCM 格式
    config_.sample_rate = 16000;

    ESP_LOGI(TAG, "ASR Client initialized");
    return true;
}

bool AliyunAsrClient::LoadConfig() {
    Settings settings("bailian", false);
    config_.api_key = settings.GetString("api_key");
    return !config_.api_key.empty();
}

std::string AliyunAsrClient::BuildRequestUrl(const std::string& base_url) {
    // 构造请求 URL (使用 GET 请求,参数在 URL 中)
    std::string url = base_url;
    url += "?appkey=default";  // 使用默认 appkey
    url += "&format=" + config_.format;
    url += "&sample_rate=" + std::to_string(config_.sample_rate);
    url += "&enable_punctuation_prediction=" + std::string(config_.enable_punctuation ? "true" : "false");
    url += "&enable_inverse_text_normalization=" + std::string(config_.enable_itn ? "true" : "false");
    
    return url;
}

bool AliyunAsrClient::RecognizeOnce(const std::vector<uint8_t>& audio_data,
                                    OnResultCallback on_result,
                                    OnErrorCallback on_error) {
    // 注意: Opus 音频需要先解码为 PCM
    // 由于 ESP32 上 Opus 解码比较复杂,这里暂时跳过 ASR
    // 建议:
    // 1. 在设备端保存原始 PCM 数据用于 ASR
    // 2. 或者在服务器端进行 ASR
    // 3. 或者集成 Opus 解码库
    
    ESP_LOGW(TAG, "ASR temporarily disabled - Opus to PCM conversion needed");
    ESP_LOGI(TAG, "Audio size: %d bytes (Opus encoded)", audio_data.size());
    
    if (on_error) {
        on_error("ASR not available - PCM format required");
    }
    
    return false;
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

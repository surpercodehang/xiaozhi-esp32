#include "bailian_api_client.h"
#include "board.h"
#include "settings.h"
#include <esp_log.h>
#include <cJSON.h>

#define TAG "BailianApiClient"

BailianApiClient::BailianApiClient() {
}

BailianApiClient::~BailianApiClient() {
}

bool BailianApiClient::Initialize() {
    if (!LoadConfig()) {
        ESP_LOGE(TAG, "Failed to load configuration");
        return false;
    }

    if (config_.api_key.empty()) {
        ESP_LOGE(TAG, "API Key is not configured");
        return false;
    }

    if (config_.app_id.empty()) {
        ESP_LOGE(TAG, "App ID is not configured");
        return false;
    }

    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();
    // 使用 30 秒超时 (索引 3 通常表示较长超时)
    http_ = network->CreateHttp(3);

    if (!http_) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    // 设置请求头
    std::string auth_header = "Bearer " + config_.api_key;
    http_->SetHeader("Authorization", auth_header.c_str());
    http_->SetHeader("Content-Type", "application/json");

    if (config_.stream) {
        http_->SetHeader("X-DashScope-SSE", "enable");
    }

    ESP_LOGI(TAG, "Initialized with App ID: %s, Stream: %s",
             config_.app_id.c_str(),
             config_.stream ? "enabled" : "disabled");
    return true;
}

bool BailianApiClient::LoadConfig() {
    // 优先从 NVS 读取
    Settings settings("bailian", false);

    config_.api_key = settings.GetString("api_key");
    config_.app_id = settings.GetString("app_id");
    config_.endpoint = settings.GetString("endpoint");
    config_.session_id = settings.GetString("session_id");
    config_.memory_id = settings.GetString("memory_id");
    config_.stream = settings.GetBool("stream", true);
    config_.incremental_output = settings.GetBool("incremental", true);
    config_.has_thoughts = settings.GetBool("has_thoughts", false);
    config_.timeout_ms = settings.GetInt("timeout_ms", 30000);

#ifdef CONFIG_API_MODE_ALIYUN_BAILIAN
    // 如果 NVS 为空,尝试从 Kconfig 读取
#ifdef CONFIG_BAILIAN_API_KEY
    if (config_.api_key.empty()) {
        config_.api_key = CONFIG_BAILIAN_API_KEY;
    }
#endif

#ifdef CONFIG_BAILIAN_APP_ID
    if (config_.app_id.empty()) {
        config_.app_id = CONFIG_BAILIAN_APP_ID;
    }
#endif

#ifdef CONFIG_BAILIAN_ENDPOINT
    if (config_.endpoint.empty()) {
        config_.endpoint = CONFIG_BAILIAN_ENDPOINT;
    }
#endif
    
#ifdef CONFIG_BAILIAN_ENABLE_STREAM
    config_.stream = CONFIG_BAILIAN_ENABLE_STREAM;
#endif

#ifdef CONFIG_BAILIAN_ENABLE_INCREMENTAL
    config_.incremental_output = CONFIG_BAILIAN_ENABLE_INCREMENTAL;
#endif

#ifdef CONFIG_BAILIAN_ENABLE_THOUGHTS
    config_.has_thoughts = CONFIG_BAILIAN_ENABLE_THOUGHTS;
#endif

#ifdef CONFIG_BAILIAN_TIMEOUT_MS
    config_.timeout_ms = CONFIG_BAILIAN_TIMEOUT_MS;
#endif
#endif

    // 如果 endpoint 仍为空,使用默认值
    if (config_.endpoint.empty()) {
        config_.endpoint = "https://dashscope.aliyuncs.com/api/v1/apps";
    }

    return !config_.api_key.empty() && !config_.app_id.empty();
}

std::string BailianApiClient::BuildRequestUrl() const {
    // https://dashscope.aliyuncs.com/api/v1/apps/{app_id}/completion
    return config_.endpoint + "/" + config_.app_id + "/completion";
}

std::string BailianApiClient::BuildRequestBody(const std::string& prompt) const {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "prompt", prompt.c_str());

    // 添加会话 ID (用于多轮对话)
    if (!config_.session_id.empty()) {
        cJSON_AddStringToObject(root, "session_id", config_.session_id.c_str());
    }

    // 添加长期记忆 ID
    if (!config_.memory_id.empty()) {
        cJSON_AddStringToObject(root, "memory_id", config_.memory_id.c_str());
    }

    // 添加参数
    if (config_.stream || config_.incremental_output || config_.has_thoughts) {
        cJSON* parameters = cJSON_CreateObject();

        if (config_.stream) {
            cJSON_AddBoolToObject(parameters, "stream", true);

            if (config_.incremental_output) {
                cJSON_AddBoolToObject(parameters, "incremental_output", true);
            }
        }

        if (config_.has_thoughts) {
            cJSON_AddBoolToObject(parameters, "has_thoughts", true);
        }

        cJSON_AddItemToObject(root, "parameters", parameters);
    }

    char* json_str = cJSON_PrintUnformatted(root);
    std::string body(json_str);
    free(json_str);
    cJSON_Delete(root);

    return body;
}

bool BailianApiClient::SendPrompt(const std::string& prompt,
                                   OnResponseCallback on_response,
                                   OnErrorCallback on_error) {
    if (!http_) {
        if (on_error) {
            on_error("HTTP client not initialized");
        }
        return false;
    }

    std::string url = BuildRequestUrl();
    std::string body = BuildRequestBody(prompt);

    ESP_LOGI(TAG, "Sending request to: %s", url.c_str());
    ESP_LOGD(TAG, "Request body: %s", body.c_str());
    ESP_LOGD(TAG, "API Key (first 10 chars): %.10s...", config_.api_key.c_str());

    // 重新设置请求头 (确保每次请求都有正确的 Header)
    std::string auth_header = "Bearer " + config_.api_key;
    http_->SetHeader("Authorization", auth_header.c_str());
    http_->SetHeader("Content-Type", "application/json");
    
    if (config_.stream) {
        http_->SetHeader("X-DashScope-SSE", "enable");
    }

    // 设置请求体
    http_->SetContent(std::move(body));

    // 发送 POST 请求
    if (!http_->Open("POST", url)) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        if (on_error) {
            on_error("Failed to open HTTP connection");
        }
        return false;
    }

    // 获取响应状态码
    int status_code = http_->GetStatusCode();
    ESP_LOGI(TAG, "HTTP status code: %d", status_code);
    
    if (status_code != 200) {
        std::string error = "HTTP error: " + std::to_string(status_code);
        ESP_LOGE(TAG, "%s", error.c_str());
        
        // 尝试读取错误响应
        std::string error_body = http_->ReadAll();
        ESP_LOGE(TAG, "Error response: %s", error_body.c_str());
        
        if (on_error) {
            on_error(error);
        }
        return false;
    }

    // 获取响应长度
    size_t content_length = http_->GetBodyLength();
    ESP_LOGI(TAG, "Response content length: %d", content_length);
    
    // 读取响应
    ESP_LOGI(TAG, "Reading response body...");
    std::string response_body = http_->ReadAll();
    ESP_LOGI(TAG, "Response body length: %d bytes", response_body.length());
    ESP_LOGD(TAG, "Response body: %s", response_body.c_str());

    if (config_.stream) {
        // 流式响应,解析 SSE
        ParseSSEResponse(response_body, on_response);
    } else {
        // 非流式响应,直接解析 JSON
        if (!ParseJsonResponse(response_body, on_response)) {
            if (on_error) {
                on_error("Failed to parse response JSON");
            }
            return false;
        }
    }

    return true;
}

void BailianApiClient::ParseSSEResponse(const std::string& data, OnResponseCallback callback) {
    // SSE 格式: data: {json}\n\n
    size_t pos = 0;

    while (pos < data.length()) {
        // 查找 "data: "
        size_t data_pos = data.find("data: ", pos);
        if (data_pos == std::string::npos) {
            break;
        }

        data_pos += 6;  // 跳过 "data: "

        // 查找行尾
        size_t end_pos = data.find("\n", data_pos);
        if (end_pos == std::string::npos) {
            break;
        }

        std::string json_str = data.substr(data_pos, end_pos - data_pos);

        // 跳过空行或 [DONE] 标记
        if (json_str.empty() || json_str == "[DONE]") {
            pos = end_pos + 1;
            continue;
        }

        // 解析 JSON
        cJSON* json = cJSON_Parse(json_str.c_str());
        if (json) {
            cJSON* output = cJSON_GetObjectItem(json, "output");
            if (output) {
                Response resp;

                // 提取文本
                cJSON* text = cJSON_GetObjectItem(output, "text");
                if (text && cJSON_IsString(text)) {
                    resp.text = text->valuestring;
                }

                // 提取会话 ID
                cJSON* session_id = cJSON_GetObjectItem(output, "session_id");
                if (session_id && cJSON_IsString(session_id)) {
                    resp.session_id = session_id->valuestring;
                    // 自动保存会话 ID
                    if (!resp.session_id.empty() && resp.session_id != config_.session_id) {
                        config_.session_id = resp.session_id;
                        ESP_LOGD(TAG, "Updated session ID: %s", config_.session_id.c_str());
                    }
                }

                // 提取结束原因
                cJSON* finish_reason = cJSON_GetObjectItem(output, "finish_reason");
                if (finish_reason && cJSON_IsString(finish_reason)) {
                    resp.finish_reason = finish_reason->valuestring;
                    resp.is_complete = (std::string(finish_reason->valuestring) == "stop");
                }

                // 提取思考过程
                if (config_.has_thoughts) {
                    cJSON* thoughts = cJSON_GetObjectItem(output, "thoughts");
                    if (thoughts && cJSON_IsString(thoughts)) {
                        resp.thoughts = thoughts->valuestring;
                    }
                }

                // 回调
                if (callback && (!resp.text.empty() || !resp.thoughts.empty() || resp.is_complete)) {
                    callback(resp);
                }
            }
            cJSON_Delete(json);
        } else {
            ESP_LOGW(TAG, "Failed to parse SSE JSON: %s", json_str.c_str());
        }

        pos = end_pos + 1;
    }
}

bool BailianApiClient::ParseJsonResponse(const std::string& json_str, OnResponseCallback callback) {
    cJSON* json = cJSON_Parse(json_str.c_str());
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return false;
    }

    bool success = false;
    cJSON* output = cJSON_GetObjectItem(json, "output");
    if (output) {
        Response resp;

        // 提取文本
        cJSON* text = cJSON_GetObjectItem(output, "text");
        if (text && cJSON_IsString(text)) {
            resp.text = text->valuestring;
        }

        // 提取会话 ID
        cJSON* session_id = cJSON_GetObjectItem(output, "session_id");
        if (session_id && cJSON_IsString(session_id)) {
            resp.session_id = session_id->valuestring;
            // 自动保存会话 ID
            if (!resp.session_id.empty()) {
                config_.session_id = resp.session_id;
                ESP_LOGD(TAG, "Updated session ID: %s", config_.session_id.c_str());
            }
        }

        // 提取结束原因
        cJSON* finish_reason = cJSON_GetObjectItem(output, "finish_reason");
        if (finish_reason && cJSON_IsString(finish_reason)) {
            resp.finish_reason = finish_reason->valuestring;
        }

        // 提取思考过程
        if (config_.has_thoughts) {
            cJSON* thoughts = cJSON_GetObjectItem(output, "thoughts");
            if (thoughts && cJSON_IsString(thoughts)) {
                resp.thoughts = thoughts->valuestring;
            }
        }

        resp.is_complete = true;

        // 回调
        if (callback) {
            callback(resp);
        }

        success = true;
    }

    cJSON_Delete(json);
    return success;
}

void BailianApiClient::SetSessionId(const std::string& session_id) {
    config_.session_id = session_id;
    ESP_LOGI(TAG, "Session ID set to: %s", session_id.c_str());
}

std::string BailianApiClient::GetSessionId() const {
    return config_.session_id;
}

void BailianApiClient::ClearSession() {
    config_.session_id.clear();
    ESP_LOGI(TAG, "Session cleared");
}

void BailianApiClient::SetMemoryId(const std::string& memory_id) {
    config_.memory_id = memory_id;
    ESP_LOGI(TAG, "Memory ID set to: %s", memory_id.c_str());
}

std::string BailianApiClient::GetMemoryId() const {
    return config_.memory_id;
}

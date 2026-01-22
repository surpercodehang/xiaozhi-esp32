#include "bailian_protocol.h"
#include "board.h"
#include "system_info.h"
#include "settings.h"
#include "application.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <cJSON.h>
#include <mbedtls/base64.h>

#define TAG "BailianProtocol"

BailianProtocol::BailianProtocol() : error_occurred_(false) {
    // 从配置中读取 API Key 和 App ID
#ifdef CONFIG_BAILIAN_API_KEY
    api_key_ = CONFIG_BAILIAN_API_KEY;
#endif
#ifdef CONFIG_BAILIAN_APP_ID
    app_id_ = CONFIG_BAILIAN_APP_ID;
#endif

    // 也支持从 NVS 设置中读取(优先级更高)
    Settings settings("bailian", false);
    std::string nvs_api_key = settings.GetString("api_key");
    std::string nvs_app_id = settings.GetString("app_id");
    
    if (!nvs_api_key.empty()) {
        api_key_ = nvs_api_key;
    }
    if (!nvs_app_id.empty()) {
        app_id_ = nvs_app_id;
    }

    ESP_LOGI(TAG, "初始化百炼应用协议");
    ESP_LOGI(TAG, "App ID: %s", app_id_.c_str());
}

BailianProtocol::~BailianProtocol() {
    CloseAudioChannel();
}

bool BailianProtocol::OpenAudioChannel() {
    if (api_key_.empty() || app_id_.empty()) {
        ESP_LOGE(TAG, "API Key 或 App ID 未配置");
        SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return false;
    }

    error_occurred_ = false;
    session_id_.clear();

    auto network = Board::GetInstance().GetNetwork();
    websocket_ = network->CreateWebSocket(1);
    if (websocket_ == nullptr) {
        ESP_LOGE(TAG, "创建 WebSocket 失败");
        return false;
    }

    // 构建完整的 WebSocket URL
    std::string url = std::string(BAILIAN_WS_URL) + "/" + app_id_;
    
    // 设置请求头
    websocket_->SetHeader("Authorization", ("Bearer " + api_key_).c_str());
    websocket_->SetHeader("Content-Type", "application/json");
    
    // 设置数据回调
    websocket_->OnData([this](const char* data, size_t len, bool binary) {
        if (binary) {
            // 二进制数据 - 音频流
            if (on_incoming_audio_ != nullptr) {
                // 百炼应用返回的是 PCM 音频,需要解码
                on_incoming_audio_(std::make_unique<AudioStreamPacket>(AudioStreamPacket{
                    .sample_rate = server_sample_rate_,
                    .frame_duration = server_frame_duration_,
                    .timestamp = 0,
                    .payload = std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + len)
                }));
            }
        } else {
            // JSON 文本数据
            auto root = cJSON_Parse(data);
            if (root != nullptr) {
                ParseServerResponse(root);
                cJSON_Delete(root);
            } else {
                ESP_LOGE(TAG, "解析 JSON 失败: %s", data);
            }
        }
        last_incoming_time_ = std::chrono::steady_clock::now();
    });

    websocket_->OnDisconnected([this]() {
        ESP_LOGI(TAG, "WebSocket 连接断开");
        if (on_audio_channel_closed_ != nullptr) {
            on_audio_channel_closed_();
        }
    });

    ESP_LOGI(TAG, "连接到百炼应用: %s", url.c_str());
    if (!websocket_->Connect(url.c_str())) {
        ESP_LOGE(TAG, "连接失败, code=%d", websocket_->GetLastError());
        SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return false;
    }

    // 发送初始消息
    if (!SendHelloMessage()) {
        return false;
    }

    return true;
}

void BailianProtocol::CloseAudioChannel() {
    if (websocket_ != nullptr) {
        // 发送会话结束消息
        if (!session_id_.empty()) {
            cJSON* close_msg = cJSON_CreateObject();
            cJSON_AddStringToObject(close_msg, "action", "finish");
            cJSON_AddStringToObject(close_msg, "session_id", session_id_.c_str());
            
            char* json_str = cJSON_PrintUnformatted(close_msg);
            if (json_str != nullptr) {
                SendText(json_str);
                free(json_str);
            }
            cJSON_Delete(close_msg);
        }
        
        websocket_.reset();
    }
    session_id_.clear();
}

bool BailianProtocol::IsAudioChannelOpened() const {
    return websocket_ != nullptr && websocket_->IsConnected() && !error_occurred_ && !IsTimeout();
}

bool BailianProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (!IsAudioChannelOpened()) {
        return false;
    }

    // 百炼应用需要 Base64 编码的音频数据
    size_t encoded_len = 0;
    mbedtls_base64_encode(nullptr, 0, &encoded_len, 
                         packet->payload.data(), packet->payload.size());
    
    std::vector<uint8_t> encoded(encoded_len);
    if (mbedtls_base64_encode(encoded.data(), encoded.size(), &encoded_len,
                              packet->payload.data(), packet->payload.size()) != 0) {
        ESP_LOGE(TAG, "Base64 编码失败");
        return false;
    }

    // 构建音频消息
    cJSON* audio_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(audio_msg, "action", "audio");
    cJSON_AddStringToObject(audio_msg, "audio", (char*)encoded.data());
    
    if (!session_id_.empty()) {
        cJSON_AddStringToObject(audio_msg, "session_id", session_id_.c_str());
    }

    char* json_str = cJSON_PrintUnformatted(audio_msg);
    bool result = false;
    if (json_str != nullptr) {
        result = SendText(json_str);
        free(json_str);
    }
    cJSON_Delete(audio_msg);

    return result;
}

bool BailianProtocol::SendText(const std::string& text) {
    if (websocket_ == nullptr || !websocket_->IsConnected()) {
        return false;
    }

    if (!websocket_->Send(text)) {
        ESP_LOGE(TAG, "发送文本失败: %s", text.c_str());
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }

    return true;
}

bool BailianProtocol::SendHelloMessage() {
    cJSON* hello = cJSON_CreateObject();
    cJSON_AddStringToObject(hello, "action", "start");
    
    // 添加输入配置
    cJSON* input = cJSON_CreateObject();
    cJSON_AddStringToObject(input, "type", "audio");
    cJSON_AddNumberToObject(input, "sample_rate", 16000);
    cJSON_AddNumberToObject(input, "channels", 1);
    cJSON_AddStringToObject(input, "format", "pcm");
    cJSON_AddItemToObject(hello, "input", input);

    // 添加输出配置
    cJSON* output = cJSON_CreateObject();
    cJSON_AddStringToObject(output, "type", "audio");
    cJSON_AddBoolToObject(output, "stream", true);
    cJSON_AddItemToObject(hello, "output", output);

    // 添加参数(如果有)
    cJSON* parameters = cJSON_CreateObject();
    cJSON_AddBoolToObject(parameters, "incremental_output", true);
    cJSON_AddItemToObject(hello, "parameters", parameters);

    char* json_str = cJSON_PrintUnformatted(hello);
    bool result = false;
    if (json_str != nullptr) {
        ESP_LOGI(TAG, "发送初始消息: %s", json_str);
        result = SendText(json_str);
        free(json_str);
    }
    cJSON_Delete(hello);

    return result;
}

void BailianProtocol::ParseServerResponse(cJSON* root) {
    // 获取事件类型
    cJSON* event = cJSON_GetObjectItem(root, "event");
    if (event == nullptr || !cJSON_IsString(event)) {
        return;
    }

    const char* event_type = event->valuestring;
    ESP_LOGI(TAG, "收到服务器事件: %s", event_type);

    // 处理会话 ID
    cJSON* session = cJSON_GetObjectItem(root, "session_id");
    if (session != nullptr && cJSON_IsString(session)) {
        session_id_ = session->valuestring;
        ESP_LOGI(TAG, "会话 ID: %s", session_id_.c_str());
    }

    // 处理不同类型的事件
    if (strcmp(event_type, "result-generated") == 0) {
        // 最终结果
        cJSON* output = cJSON_GetObjectItem(root, "output");
        if (output != nullptr) {
            cJSON* text = cJSON_GetObjectItem(output, "text");
            if (text != nullptr && cJSON_IsString(text)) {
                ESP_LOGI(TAG, "AI回复: %s", text->valuestring);
            }

            // 处理音频数据
            cJSON* audio = cJSON_GetObjectItem(output, "audio");
            if (audio != nullptr && cJSON_IsString(audio)) {
                // Base64 解码音频
                const char* audio_b64 = audio->valuestring;
                size_t audio_len = strlen(audio_b64);
                size_t decoded_len = 0;
                
                mbedtls_base64_decode(nullptr, 0, &decoded_len, 
                                    (uint8_t*)audio_b64, audio_len);
                
                std::vector<uint8_t> decoded(decoded_len);
                if (mbedtls_base64_decode(decoded.data(), decoded.size(), &decoded_len,
                                         (uint8_t*)audio_b64, audio_len) == 0) {
                    if (on_incoming_audio_ != nullptr) {
                        on_incoming_audio_(std::make_unique<AudioStreamPacket>(AudioStreamPacket{
                            .sample_rate = server_sample_rate_,
                            .frame_duration = server_frame_duration_,
                            .timestamp = 0,
                            .payload = decoded
                        }));
                    }
                }
            }
        }
    } else if (strcmp(event_type, "error") == 0) {
        // 错误事件
        cJSON* message = cJSON_GetObjectItem(root, "message");
        const char* error_msg = message != nullptr && cJSON_IsString(message) 
                              ? message->valuestring : "未知错误";
        ESP_LOGE(TAG, "服务器错误: %s", error_msg);
        SetError(Lang::Strings::SERVER_ERROR);
        error_occurred_ = true;
    }

    // 转发 JSON 给上层
    if (on_incoming_json_ != nullptr) {
        on_incoming_json_(root);
    }
}

bool BailianProtocol::IsTimeout() const {
    if (last_incoming_time_.time_since_epoch().count() == 0) {
        return false;
    }
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_incoming_time_);
    return duration.count() > 30; // 30秒超时
}

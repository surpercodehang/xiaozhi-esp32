#include "bailian_protocol.h"
#include "settings.h"
#include <esp_log.h>
#include <cJSON.h>

#define TAG "BailianProtocol"

BailianProtocol::BailianProtocol() {
    client_ = std::make_unique<BailianApiClient>();
    asr_client_ = std::make_unique<AliyunAsrClient>();
    tts_client_ = std::make_unique<AliyunTtsClient>();
}

BailianProtocol::~BailianProtocol() {
}

bool BailianProtocol::Start() {
    ESP_LOGI(TAG, "Starting Bailian Protocol");

    // 获取 API Key
    Settings settings("bailian", false);
    std::string api_key = settings.GetString("api_key");
    
#ifdef CONFIG_BAILIAN_API_KEY
    if (api_key.empty()) {
        api_key = CONFIG_BAILIAN_API_KEY;
    }
#endif

    if (api_key.empty()) {
        ESP_LOGE(TAG, "API Key not configured");
        SetError("API Key not configured");
        return false;
    }

    // 初始化百炼 LLM 客户端
    if (!client_->Initialize()) {
        ESP_LOGE(TAG, "Failed to initialize Bailian API client");
        SetError("Failed to initialize Bailian API");
        return false;
    }

    // 初始化 ASR 客户端
    if (!asr_client_->Initialize(api_key)) {
        ESP_LOGW(TAG, "Failed to initialize ASR client (will retry later)");
    } else {
        ESP_LOGI(TAG, "ASR client initialized");
    }

    // 初始化 TTS 客户端
    if (!tts_client_->Initialize(api_key)) {
        ESP_LOGW(TAG, "Failed to initialize TTS client (will retry later)");
    } else {
        ESP_LOGI(TAG, "TTS client initialized");
    }

    if (on_connected_) {
        on_connected_();
    }

    ESP_LOGI(TAG, "Bailian Protocol started successfully");
    return true;
}

bool BailianProtocol::OpenAudioChannel() {
    ESP_LOGI(TAG, "Opening audio channel");

    if (channel_opened_) {
        ESP_LOGW(TAG, "Audio channel already opened");
        return true;
    }

    // 清空累积的数据
    accumulated_text_.clear();
    audio_buffer_.clear();

    // 标记通道已打开
    channel_opened_ = true;

    // 通知通道已打开
    if (on_audio_channel_opened_) {
        on_audio_channel_opened_();
    }

    ESP_LOGI(TAG, "Audio channel opened");
    return true;
}

void BailianProtocol::CloseAudioChannel() {
    ESP_LOGI(TAG, "Closing audio channel");

    if (!channel_opened_) {
        ESP_LOGW(TAG, "Audio channel already closed");
        return;
    }

    // 如果有累积的音频数据,先进行 ASR 识别
    if (!audio_buffer_.empty() && is_listening_) {
        ESP_LOGI(TAG, "Processing accumulated audio before closing (%d bytes)", audio_buffer_.size());
        
        asr_client_->RecognizeOnce(
            audio_buffer_,
            [this](const AliyunAsrClient::Response& resp) {
                if (!resp.text.empty()) {
                    ESP_LOGI(TAG, "Final ASR result: %s", resp.text.c_str());
                    // 将识别结果发送到 LLM
                    SendText(resp.text);
                }
            },
            [](const std::string& error) {
                ESP_LOGE(TAG, "ASR error: %s", error.c_str());
            }
        );
    }

    // 标记通道已关闭
    channel_opened_ = false;
    is_listening_ = false;

    // 清空累积的数据
    accumulated_text_.clear();
    audio_buffer_.clear();

    // 通知通道已关闭
    if (on_audio_channel_closed_) {
        on_audio_channel_closed_();
    }

    ESP_LOGI(TAG, "Audio channel closed");
}

bool BailianProtocol::IsAudioChannelOpened() const {
    return channel_opened_;
}

bool BailianProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (!channel_opened_ || !is_listening_) {
        return false;
    }

    if (!packet || packet->payload.empty()) {
        return false;
    }

    // 累积音频数据 (Opus 编码)
    audio_buffer_.insert(audio_buffer_.end(), 
                         packet->payload.begin(), 
                         packet->payload.end());

    ESP_LOGD(TAG, "Audio packet received: %d bytes, total: %d bytes", 
             packet->payload.size(), audio_buffer_.size());

    // 当累积足够的音频数据时(约 3 秒),进行一次 ASR 识别
    // 这里简单地设置一个阈值,实际应用中可以根据 VAD 判断
    const size_t MAX_BUFFER_SIZE = 48000; // 约 3 秒的 Opus 数据
    
    if (audio_buffer_.size() >= MAX_BUFFER_SIZE) {
        ESP_LOGI(TAG, "Buffer full, triggering ASR recognition");
        
        // 复制当前缓冲区用于识别
        std::vector<uint8_t> audio_data = audio_buffer_;
        
        // 清空缓冲区以接收新数据
        audio_buffer_.clear();
        
        // 异步进行 ASR 识别
        asr_client_->RecognizeOnce(
            audio_data,
            [this](const AliyunAsrClient::Response& resp) {
                if (!resp.text.empty()) {
                    ESP_LOGI(TAG, "ASR result: %s", resp.text.c_str());
                    // 将识别结果发送到 LLM
                    SendText(resp.text);
                }
            },
            [](const std::string& error) {
                ESP_LOGE(TAG, "ASR error: %s", error.c_str());
            }
        );
    }

    return true;
}

bool BailianProtocol::SendText(const std::string& text) {
    if (!channel_opened_) {
        ESP_LOGW(TAG, "Audio channel not opened");
        return false;
    }

    ESP_LOGI(TAG, "Sending text to Bailian API: %s", text.c_str());

    // 清空累积的文本
    accumulated_text_.clear();

    // 发送请求到百炼 API
    bool success = client_->SendPrompt(
        text,
        [this](const BailianApiClient::Response& response) {
            HandleResponse(response);
        },
        [this](const std::string& error) {
            HandleError(error);
        }
    );

    if (!success) {
        ESP_LOGE(TAG, "Failed to send text to Bailian API");
        SetError("Failed to send request");
        return false;
    }

    return true;
}

void BailianProtocol::SendWakeWordDetected(const std::string& wake_word) {
    ESP_LOGI(TAG, "Wake word detected: %s", wake_word.c_str());

    // 构造唤醒事件消息
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "wake_word");
    cJSON_AddStringToObject(json, "wake_word", wake_word.c_str());

    // 通知应用层
    if (on_incoming_json_) {
        on_incoming_json_(json);
    }

    cJSON_Delete(json);
}

void BailianProtocol::SendStartListening(ListeningMode mode) {
    ESP_LOGI(TAG, "Start listening, mode: %d", mode);

    is_listening_ = true;

    // 构造开始监听事件消息
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "listening");
    cJSON_AddBoolToObject(json, "is_listening", true);

    // 通知应用层
    if (on_incoming_json_) {
        on_incoming_json_(json);
    }

    cJSON_Delete(json);
}

void BailianProtocol::SendStopListening() {
    ESP_LOGI(TAG, "Stop listening");

    is_listening_ = false;

    // 构造停止监听事件消息
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "listening");
    cJSON_AddBoolToObject(json, "is_listening", false);

    // 通知应用层
    if (on_incoming_json_) {
        on_incoming_json_(json);
    }

    cJSON_Delete(json);
}

void BailianProtocol::SendMcpMessage(const std::string& message) {
    ESP_LOGI(TAG, "Sending MCP message: %s", message.c_str());

    // 注意: 阿里云百炼 API 支持自定义插件
    // 可以通过 biz_params.user_defined_params 传递 MCP 消息
    // 这里暂时通过 on_incoming_json_ 直接回传

    cJSON* json = cJSON_Parse(message.c_str());
    if (json) {
        // 标记为 MCP 消息
        cJSON_AddStringToObject(json, "type", "mcp");

        // 通知应用层
        if (on_incoming_json_) {
            on_incoming_json_(json);
        }

        cJSON_Delete(json);
    } else {
        ESP_LOGE(TAG, "Failed to parse MCP message");
    }
}

void BailianProtocol::HandleResponse(const BailianApiClient::Response& response) {
    ESP_LOGD(TAG, "Received response from Bailian API");

    // 更新会话 ID
    if (!response.session_id.empty()) {
        session_id_ = response.session_id;
    }

    // 处理思考过程
    if (!response.thoughts.empty()) {
        ESP_LOGI(TAG, "Thoughts: %s", response.thoughts.c_str());

        // 构造思考过程消息
        cJSON* json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "type", "thoughts");
        cJSON_AddStringToObject(json, "text", response.thoughts.c_str());

        if (on_incoming_json_) {
            on_incoming_json_(json);
        }

        cJSON_Delete(json);
    }

    // 处理回复文本
    if (!response.text.empty()) {
        // 累积文本(流式输出时)
        accumulated_text_ += response.text;

        ESP_LOGI(TAG, "Response text: %s", response.text.c_str());

        // 构造聊天消息
        cJSON* json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "type", "chat");
        cJSON_AddStringToObject(json, "role", "assistant");
        cJSON_AddStringToObject(json, "text", response.text.c_str());
        cJSON_AddStringToObject(json, "session_id", session_id_.c_str());

        if (response.is_complete) {
            cJSON_AddBoolToObject(json, "is_complete", true);
            cJSON_AddStringToObject(json, "finish_reason", response.finish_reason.c_str());
        }

        if (on_incoming_json_) {
            on_incoming_json_(json);
        }

        cJSON_Delete(json);
    }

    // 如果响应完成,进行 TTS 合成
    if (response.is_complete && !accumulated_text_.empty()) {
        ESP_LOGI(TAG, "Response completed, synthesizing speech...");

        // 发送 TTS 开始事件
        {
            cJSON* json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "type", "tts");
            cJSON_AddStringToObject(json, "state", "start");
            if (on_incoming_json_) {
                on_incoming_json_(json);
            }
            cJSON_Delete(json);
        }

        // 调用 TTS 合成
        std::string text_to_speak = accumulated_text_;
        tts_client_->Synthesize(
            text_to_speak,
            [this](const AliyunTtsClient::Response& tts_resp) {
                if (tts_resp.success && !tts_resp.audio_data.empty()) {
                    ESP_LOGI(TAG, "TTS success, audio size: %d bytes", tts_resp.audio_data.size());
                    
                    // 将音频数据转换为 AudioStreamPacket 并发送
                    auto packet = std::make_unique<AudioStreamPacket>();
                    packet->sample_rate = tts_resp.sample_rate;
                    packet->frame_duration = 60; // Opus 帧时长
                    packet->payload = tts_resp.audio_data;
                    
                    if (on_incoming_audio_) {
                        on_incoming_audio_(std::move(packet));
                    }

                    // 发送 TTS 完成事件
                    cJSON* json = cJSON_CreateObject();
                    cJSON_AddStringToObject(json, "type", "tts");
                    cJSON_AddStringToObject(json, "state", "stop");
                    if (on_incoming_json_) {
                        on_incoming_json_(json);
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGE(TAG, "TTS failed");
                }
            },
            [this](const std::string& error) {
                ESP_LOGE(TAG, "TTS error: %s", error.c_str());
                
                // 发送 TTS 错误事件
                cJSON* json = cJSON_CreateObject();
                cJSON_AddStringToObject(json, "type", "tts");
                cJSON_AddStringToObject(json, "state", "error");
                cJSON_AddStringToObject(json, "error", error.c_str());
                if (on_incoming_json_) {
                    on_incoming_json_(json);
                }
                cJSON_Delete(json);
            }
        );

        // 构造完成事件
        cJSON* json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "type", "completion");
        cJSON_AddStringToObject(json, "text", accumulated_text_.c_str());
        cJSON_AddStringToObject(json, "finish_reason", response.finish_reason.c_str());

        if (on_incoming_json_) {
            on_incoming_json_(json);
        }

        cJSON_Delete(json);

        // 清空累积的文本
        accumulated_text_.clear();
    }

    // 更新最后接收时间
    last_incoming_time_ = std::chrono::steady_clock::now();
}

void BailianProtocol::HandleError(const std::string& error) {
    ESP_LOGE(TAG, "Bailian API error: %s", error.c_str());

    // 设置错误状态
    SetError(error);

    // 构造错误消息
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "type", "error");
    cJSON_AddStringToObject(json, "message", error.c_str());

    if (on_incoming_json_) {
        on_incoming_json_(json);
    }

    cJSON_Delete(json);
}

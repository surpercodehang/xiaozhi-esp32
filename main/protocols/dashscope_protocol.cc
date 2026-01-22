#include "dashscope_protocol.h"
#include "board.h"
#include "system_info.h"
#include "application.h"
#include "settings.h"

#include <cstring>
#include <cJSON.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include "assets/lang_config.h"

#define TAG "Dashscope"

// 阿里云 API 端点
#define DASHSCOPE_APP_URL "https://dashscope.aliyuncs.com/api/v1/apps/%s/completion"
#define DASHSCOPE_ASR_URL "https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/asr"
#define DASHSCOPE_TTS_URL "https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/tts"

DashscopeProtocol::DashscopeProtocol() 
    : is_channel_opened_(false),
      asr_task_handle_(nullptr),
      tts_task_handle_(nullptr),
      audio_queue_(nullptr),
      event_group_handle_(nullptr) {
    
    event_group_handle_ = xEventGroupCreate();
    audio_queue_ = xQueueCreate(30, sizeof(AudioStreamPacket*));
}

DashscopeProtocol::~DashscopeProtocol() {
    CloseAudioChannel();
    if (event_group_handle_) {
        vEventGroupDelete(event_group_handle_);
    }
    if (audio_queue_) {
        vQueueDelete(audio_queue_);
    }
}

bool DashscopeProtocol::Start() {
    // 从配置中读取 API Key 和 App ID
    Settings settings("dashscope", false);
    api_key_ = settings.GetString("api_key");
    app_id_ = settings.GetString("app_id");
    
    if (api_key_.empty()) {
        api_key_ = "sk-3c373c69431947e781a68fd9dad86ccd"; // 默认值
    }
    if (app_id_.empty()) {
        app_id_ = "c897a3567b374b839bb0b1974aebc548"; // 默认值
    }
    
    ESP_LOGI(TAG, "Dashscope protocol started with app_id: %s", app_id_.c_str());
    return true;
}

bool DashscopeProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (!is_channel_opened_ || !packet) {
        return false;
    }
    
    // 将音频数据发送到队列
    AudioStreamPacket* raw_packet = packet.release();
    if (xQueueSend(audio_queue_, &raw_packet, 0) != pdTRUE) {
        delete raw_packet;
        return false;
    }
    
    return true;
}

bool DashscopeProtocol::SendText(const std::string& text) {
    // DashscopeProtocol 不需要实现 SendText，使用 HTTP API
    return true;
}

bool DashscopeProtocol::IsAudioChannelOpened() const {
    return is_channel_opened_;
}

void DashscopeProtocol::CloseAudioChannel() {
    if (!is_channel_opened_) {
        return;
    }
    
    is_channel_opened_ = false;
    
    // 停止任务
    if (asr_task_handle_) {
        vTaskDelete(asr_task_handle_);
        asr_task_handle_ = nullptr;
    }
    if (tts_task_handle_) {
        vTaskDelete(tts_task_handle_);
        tts_task_handle_ = nullptr;
    }
    
    // 清空队列
    AudioStreamPacket* packet;
    while (xQueueReceive(audio_queue_, &packet, 0) == pdTRUE) {
        delete packet;
    }
    
    audio_buffer_.clear();
    session_id_internal_.clear();
    
    if (on_audio_channel_closed_) {
        on_audio_channel_closed_();
    }
}

bool DashscopeProtocol::OpenAudioChannel() {
    if (is_channel_opened_) {
        return true;
    }
    
    error_occurred_ = false;
    is_channel_opened_ = true;
    
    // 生成新的 session_id
    char session_buf[64];
    snprintf(session_buf, sizeof(session_buf), "session_%lld", esp_timer_get_time());
    session_id_internal_ = session_buf;
    session_id_ = session_id_internal_;
    
    // 创建 ASR 处理任务
    xTaskCreate(AsrTaskFunction, "asr_task", 8192, this, 5, &asr_task_handle_);
    
    ESP_LOGI(TAG, "Audio channel opened with session: %s", session_id_internal_.c_str());
    
    if (on_audio_channel_opened_) {
        on_audio_channel_opened_();
    }
    
    return true;
}

void DashscopeProtocol::AsrTaskFunction(void* param) {
    DashscopeProtocol* protocol = static_cast<DashscopeProtocol*>(param);
    
    std::vector<uint8_t> accumulated_audio;
    const size_t MIN_AUDIO_SIZE = 16000 * 2; // 至少1秒的16kHz PCM数据
    
    while (protocol->is_channel_opened_) {
        AudioStreamPacket* packet = nullptr;
        if (xQueueReceive(protocol->audio_queue_, &packet, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (packet) {
                // 累积音频数据
                accumulated_audio.insert(accumulated_audio.end(), 
                                        packet->payload.begin(), 
                                        packet->payload.end());
                delete packet;
                
                // 当累积足够的音频数据时,发送到ASR
                if (accumulated_audio.size() >= MIN_AUDIO_SIZE) {
                    protocol->SendToAsr(accumulated_audio);
                    accumulated_audio.clear();
                }
            }
        }
    }
    
    // 处理剩余的音频数据
    if (!accumulated_audio.empty()) {
        protocol->SendToAsr(accumulated_audio);
    }
    
    vTaskDelete(NULL);
}

void DashscopeProtocol::TtsTaskFunction(void* param) {
    DashscopeProtocol* protocol = static_cast<DashscopeProtocol*>(param);
    // TTS任务实现将在 ProcessTtsStream 中处理
    vTaskDelete(NULL);
}

bool DashscopeProtocol::SendToAsr(const std::vector<uint8_t>& audio_data) {
    ESP_LOGI(TAG, "Sending %d bytes to ASR", audio_data.size());
    
    // 将 Opus 数据解码为 PCM
    std::vector<int16_t> pcm_data;
    if (!DecodeOpusToWav(audio_data, pcm_data)) {
        ESP_LOGE(TAG, "Failed to decode Opus audio");
        return false;
    }
    
    // 调用阿里云语音识别API (简化版本，实际需要使用WebSocket流式识别)
    // 这里使用HTTP POST模拟
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "format", "pcm");
    cJSON_AddNumberToObject(root, "sample_rate", 16000);
    cJSON_AddStringToObject(root, "enable_intermediate_result", "false");
    cJSON_AddStringToObject(root, "enable_punctuation_prediction", "true");
    
    char* json_str = cJSON_PrintUnformatted(root);
    std::string request_body = json_str;
    cJSON_free(json_str);
    cJSON_Delete(root);
    
    // 模拟ASR响应 - 在实际实现中需要调用真实的阿里云ASR API
    // 这里简化为直接触发LLM
    std::string recognized_text = "你好"; // 模拟识别结果
    
    ESP_LOGI(TAG, "ASR Result: %s", recognized_text.c_str());
    
    // 触发 STT 回调
    if (on_incoming_json_) {
        cJSON* stt_root = cJSON_CreateObject();
        cJSON_AddStringToObject(stt_root, "type", "stt");
        cJSON_AddStringToObject(stt_root, "session_id", session_id_.c_str());
        cJSON_AddStringToObject(stt_root, "text", recognized_text.c_str());
        
        on_incoming_json_(stt_root);
        cJSON_Delete(stt_root);
    }
    
    // 发送到 LLM
    return SendToLlm(recognized_text);
}

bool DashscopeProtocol::SendToLlm(const std::string& text) {
    ESP_LOGI(TAG, "Sending to LLM: %s", text.c_str());
    
    // 构建请求URL
    char url[256];
    snprintf(url, sizeof(url), DASHSCOPE_APP_URL, app_id_.c_str());
    
    // 构建请求体
    cJSON* root = cJSON_CreateObject();
    cJSON* input = cJSON_CreateObject();
    cJSON_AddStringToObject(input, "prompt", text.c_str());
    cJSON_AddItemToObject(root, "input", input);
    
    cJSON* parameters = cJSON_CreateObject();
    cJSON_AddBoolToObject(parameters, "incremental_output", true);
    cJSON_AddItemToObject(root, "parameters", parameters);
    
    if (!session_id_internal_.empty()) {
        cJSON_AddStringToObject(root, "session_id", session_id_internal_.c_str());
    }
    
    char* json_str = cJSON_PrintUnformatted(root);
    std::string request_body = json_str;
    cJSON_free(json_str);
    cJSON_Delete(root);
    
    // 发起流式请求
    std::string response = HttpPostStream(url, request_body);
    
    if (response.empty()) {
        ESP_LOGE(TAG, "Failed to get LLM response");
        return false;
    }
    
    // 解析响应
    cJSON* response_root = cJSON_Parse(response.c_str());
    if (!response_root) {
        ESP_LOGE(TAG, "Failed to parse LLM response");
        return false;
    }
    
    // 提取 session_id
    cJSON* output = cJSON_GetObjectItem(response_root, "output");
    if (output) {
        cJSON* session_id_obj = cJSON_GetObjectItem(output, "session_id");
        if (cJSON_IsString(session_id_obj)) {
            session_id_internal_ = session_id_obj->valuestring;
        }
        
        cJSON* text_obj = cJSON_GetObjectItem(output, "text");
        if (cJSON_IsString(text_obj)) {
            std::string llm_text = text_obj->valuestring;
            ESP_LOGI(TAG, "LLM Response: %s", llm_text.c_str());
            
            // 发送到 TTS
            RequestTts(llm_text);
        }
    }
    
    cJSON_Delete(response_root);
    return true;
}

bool DashscopeProtocol::RequestTts(const std::string& text) {
    ESP_LOGI(TAG, "Requesting TTS for: %s", text.c_str());
    
    // 触发 TTS 开始事件
    if (on_incoming_json_) {
        cJSON* tts_start = cJSON_CreateObject();
        cJSON_AddStringToObject(tts_start, "type", "tts");
        cJSON_AddStringToObject(tts_start, "session_id", session_id_.c_str());
        cJSON_AddStringToObject(tts_start, "state", "start");
        
        on_incoming_json_(tts_start);
        cJSON_Delete(tts_start);
    }
    
    // 发送句子开始事件
    if (on_incoming_json_) {
        cJSON* sentence_start = cJSON_CreateObject();
        cJSON_AddStringToObject(sentence_start, "type", "tts");
        cJSON_AddStringToObject(sentence_start, "session_id", session_id_.c_str());
        cJSON_AddStringToObject(sentence_start, "state", "sentence_start");
        cJSON_AddStringToObject(sentence_start, "text", text.c_str());
        
        on_incoming_json_(sentence_start);
        cJSON_Delete(sentence_start);
    }
    
    // 创建 TTS 任务处理
    xTaskCreate([](void* param) {
        auto* protocol = static_cast<DashscopeProtocol*>(param);
        protocol->ProcessTtsStream(*static_cast<std::string*>(param + sizeof(void*)));
        vTaskDelete(NULL);
    }, "tts_stream", 8192, this, 5, nullptr);
    
    return true;
}

void DashscopeProtocol::ProcessTtsStream(const std::string& text) {
    // 调用阿里云 TTS API
    // 这里简化实现，实际需要调用真实的TTS服务
    
    // 模拟生成音频数据
    // 在实际实现中，需要调用阿里云TTS API并将返回的音频流式传输
    
    // 发送 TTS 结束事件
    if (on_incoming_json_) {
        cJSON* tts_stop = cJSON_CreateObject();
        cJSON_AddStringToObject(tts_stop, "type", "tts");
        cJSON_AddStringToObject(tts_stop, "session_id", session_id_.c_str());
        cJSON_AddStringToObject(tts_stop, "state", "stop");
        
        on_incoming_json_(tts_stop);
        cJSON_Delete(tts_stop);
    }
}

std::string DashscopeProtocol::HttpPost(const std::string& url, const std::string& json_data, const std::string& content_type) {
    std::string response_data;
    
    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 30000;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return "";
    }
    
    // 设置请求头
    std::string auth_header = "Bearer " + api_key_;
    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
    esp_http_client_set_header(client, "Content-Type", content_type.c_str());
    esp_http_client_set_header(client, "X-DashScope-SSE", "enable");
    
    esp_http_client_set_post_field(client, json_data.c_str(), json_data.length());
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", status_code, content_length);
        
        if (status_code == 200) {
            char buffer[512];
            int read_len;
            while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1)) > 0) {
                buffer[read_len] = '\0';
                response_data += buffer;
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return response_data;
}

std::string DashscopeProtocol::HttpPostStream(const std::string& url, const std::string& json_data) {
    // 简化实现，实际需要处理流式响应
    return HttpPost(url, json_data);
}

bool DashscopeProtocol::DecodeOpusToWav(const std::vector<uint8_t>& opus_data, std::vector<int16_t>& pcm_data) {
    // 简化实现: 音频数据已经在 AudioService 中处理
    // 这里直接假设 opus_data 是原始 PCM 数据
    // 实际使用时,AudioService 会处理 Opus 编解码
    
    // 将 uint8_t 转换为 int16_t
    size_t samples = opus_data.size() / 2;
    pcm_data.resize(samples);
    
    for (size_t i = 0; i < samples; i++) {
        pcm_data[i] = (int16_t)(opus_data[i * 2] | (opus_data[i * 2 + 1] << 8));
    }
    
    ESP_LOGI(TAG, "Decoded %d samples from %d bytes", samples, opus_data.size());
    return true;
}

bool DashscopeProtocol::EncodePcmToOpus(const std::vector<int16_t>& pcm_data, std::vector<uint8_t>& opus_data) {
    // 简化实现: 音频数据已经在 AudioService 中处理
    // 这里直接将 PCM 转换为字节流
    // 实际使用时,AudioService 会处理 Opus 编解码
    
    opus_data.resize(pcm_data.size() * 2);
    
    for (size_t i = 0; i < pcm_data.size(); i++) {
        opus_data[i * 2] = pcm_data[i] & 0xFF;
        opus_data[i * 2 + 1] = (pcm_data[i] >> 8) & 0xFF;
    }
    
    ESP_LOGI(TAG, "Encoded %d samples to %d bytes", pcm_data.size(), opus_data.size());
    return true;
}

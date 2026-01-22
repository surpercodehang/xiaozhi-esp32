#ifndef _DASHSCOPE_PROTOCOL_H_
#define _DASHSCOPE_PROTOCOL_H_

#include "protocol.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <memory>
#include <string>
#include <vector>

// 阿里云百炼应用协议
class DashscopeProtocol : public Protocol {
public:
    DashscopeProtocol();
    ~DashscopeProtocol();

    bool Start() override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    bool OpenAudioChannel() override;
    void CloseAudioChannel() override;
    bool IsAudioChannelOpened() const override;

private:
    // 配置参数
    std::string api_key_;
    std::string app_id_;
    std::string session_id_internal_;
    bool is_channel_opened_;
    
    // FreeRTOS 资源
    TaskHandle_t asr_task_handle_;
    TaskHandle_t tts_task_handle_;
    QueueHandle_t audio_queue_;
    EventGroupHandle_t event_group_handle_;
    
    // 音频缓冲
    std::vector<uint8_t> audio_buffer_;
    
    // 任务函数
    static void AsrTaskFunction(void* param);
    static void TtsTaskFunction(void* param);
    
    // HTTP 请求辅助函数
    std::string HttpPost(const std::string& url, const std::string& json_data, const std::string& content_type = "application/json");
    std::string HttpPostStream(const std::string& url, const std::string& json_data);
    
    // 业务逻辑函数
    bool SendToAsr(const std::vector<uint8_t>& audio_data);
    bool SendToLlm(const std::string& text);
    bool RequestTts(const std::string& text);
    
    // Opus 音频处理
    bool DecodeOpusToWav(const std::vector<uint8_t>& opus_data, std::vector<int16_t>& pcm_data);
    bool EncodePcmToOpus(const std::vector<int16_t>& pcm_data, std::vector<uint8_t>& opus_data);
    
    bool SendText(const std::string& text) override;
    void ProcessAudioStream();
    void ProcessTtsStream(const std::string& text);
};

#endif // _DASHSCOPE_PROTOCOL_H_

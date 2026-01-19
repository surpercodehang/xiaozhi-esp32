#ifndef BAILIAN_PROTOCOL_H
#define BAILIAN_PROTOCOL_H

#include "protocol.h"
#include "bailian_api_client.h"
#include "aliyun_asr_client.h"
#include "aliyun_tts_client.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @brief 阿里云百炼协议适配器
 * 
 * 将阿里云百炼 API 适配为 Protocol 接口,使其可以无缝替换原有的 WebSocket/MQTT 协议
 */
class BailianProtocol : public Protocol {
public:
    BailianProtocol();
    ~BailianProtocol() override;

    bool Start() override;
    bool OpenAudioChannel() override;
    void CloseAudioChannel() override;
    bool IsAudioChannelOpened() const override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    void SendWakeWordDetected(const std::string& wake_word) override;
    void SendStartListening(ListeningMode mode) override;
    void SendStopListening() override;
    void SendMcpMessage(const std::string& message) override;

protected:
    bool SendText(const std::string& text) override;

private:
    std::unique_ptr<BailianApiClient> client_;
    std::unique_ptr<AliyunAsrClient> asr_client_;
    std::unique_ptr<AliyunTtsClient> tts_client_;
    
    bool channel_opened_ = false;
    bool is_listening_ = false;
    std::string accumulated_text_;      // 累积的文本(用于拼接流式输出)
    std::vector<uint8_t> audio_buffer_; // 累积的音频数据

    /**
     * @brief 处理百炼 API 响应
     * @param response API 响应
     */
    void HandleResponse(const BailianApiClient::Response& response);

    /**
     * @brief 处理错误
     * @param error 错误信息
     */
    void HandleError(const std::string& error);
};

#endif // BAILIAN_PROTOCOL_H

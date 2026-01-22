#ifndef BAILIAN_PROTOCOL_H
#define BAILIAN_PROTOCOL_H

#include "protocol.h"
#include "websocket.h"
#include <memory>
#include <string>
#include <chrono>

/**
 * @brief 阿里云百炼应用协议类
 * 
 * 这个类实现了直接使用阿里云百炼应用 API Key + App ID 的通信协议,
 * 无需通过 xiaozhi.me 官网配置智能体。
 * 
 * 使用方法:
 * 1. 在 menuconfig 中配置:
 *    - CONFIG_BAILIAN_API_KEY: 你的百炼 API Key
 *    - CONFIG_BAILIAN_APP_ID: 你的百炼应用 ID
 * 2. 选择协议类型为 PROTOCOL_BAILIAN
 * 
 * 特点:
 * - 支持多轮对话 (session_id)
 * - 支持流式输出
 * - 自动音频格式转换
 */
class BailianProtocol : public Protocol {
public:
    BailianProtocol();
    ~BailianProtocol();

    bool OpenAudioChannel() override;
    void CloseAudioChannel() override;
    bool IsAudioChannelOpened() const override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    bool SendText(const std::string& text) override;

private:
    std::unique_ptr<WebSocket> websocket_;
    std::string api_key_;
    std::string app_id_;
    std::string session_id_;
    bool error_occurred_;
    std::chrono::steady_clock::time_point last_incoming_time_;
    
    // 百炼应用 WebSocket 地址
    static constexpr const char* BAILIAN_WS_URL = "wss://dashscope.aliyuncs.com/api-ws/v1/apps";
    
    // 音频参数
    int server_sample_rate_ = 24000;
    int server_frame_duration_ = 20;
    
    bool SendHelloMessage();
    void ParseServerResponse(cJSON* root);
    bool IsTimeout() const;
};

#endif // BAILIAN_PROTOCOL_H

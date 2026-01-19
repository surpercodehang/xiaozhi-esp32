#ifndef DASHSCOPE_PROTOCOL_H
#define DASHSCOPE_PROTOCOL_H

#include "protocol.h"
#include <http.h>
#include <string>
#include <memory>
#include <queue>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

class DashScopeProtocol : public Protocol {
public:
    DashScopeProtocol();
    ~DashScopeProtocol();

    bool Start() override;
    bool OpenAudioChannel() override;
    void CloseAudioChannel() override;
    bool IsAudioChannelOpened() const override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    void SendWakeWordDetected(const std::string& wake_word) override;
    void SendStartListening(ListeningMode mode) override;
    void SendStopListening() override;
    void SendAbortSpeaking(AbortReason reason) override;
    void SendMcpMessage(const std::string& message) override;

protected:
    bool SendText(const std::string& text) override;

private:
    struct ConversationMessage {
        std::string role;
        std::string content;
    };

    bool InitializeHttpClient();
    bool SendConversationRequest(const std::string& prompt);
    bool SendStreamingConversationRequest(const std::string& prompt);
    void ProcessStreamingResponse(const std::string& response_data);
    void HandleAudioResponse(const std::string& audio_data);
    void AddMessageToHistory(const std::string& role, const std::string& content);
    std::string BuildRequestJson(const std::string& prompt) const;
    bool IsAudioChannelTimeout() const;

    std::unique_ptr<Http> http_client_;
    std::string api_key_;
    std::string app_id_;
    std::string base_url_;
    std::string session_id_;
    std::vector<ConversationMessage> conversation_history_;
    EventGroupHandle_t event_group_;
    bool audio_channel_opened_;
    std::chrono::time_point<std::chrono::steady_clock> audio_channel_open_time_;

    static const char* const TAG;
    static const int AUDIO_CHANNEL_TIMEOUT_MS = 30000; // 30 seconds
};

#endif // DASHSCOPE_PROTOCOL_H
#include "websocket_asr_client.h"
#include <esp_log.h>

static const char* TAG = "WebSocketAsrClient";

WebsocketAsrClient::WebsocketAsrClient() {
}

WebsocketAsrClient::~WebsocketAsrClient() {
    Deinitialize();
}

bool WebsocketAsrClient::Initialize(const std::string& server_url) {
    server_url_ = server_url;
    
    ESP_LOGW(TAG, "WebSocket ASR client is currently disabled");
    ESP_LOGW(TAG, "Reason: ESP-IDF WebSocket component requires additional configuration");
    ESP_LOGW(TAG, "Configured URL: %s (not used)", server_url.c_str());
    
    // TODO: 实现完整的 WebSocket 客户端
    // 需要添加 esp_websocket_client 组件依赖
    // 或者使用 HTTP 长轮询作为替代方案
    
    return false;  // 暂时返回 false,表示未连接
}

void WebsocketAsrClient::Deinitialize() {
    // Nothing to do for now
}

bool WebsocketAsrClient::SendAudio(const std::vector<uint8_t>& opus_audio, OnAsrResultCallback on_result, OnErrorCallback on_error) {
    ESP_LOGW(TAG, "SendAudio called but WebSocket is disabled (%zu bytes)", opus_audio.size());
    
    if (on_error) {
        on_error("WebSocket ASR is currently disabled");
    }
    
    return false;
}

#ifndef WEBSOCKET_ASR_CLIENT_H
#define WEBSOCKET_ASR_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include <vector>

// WebSocket ASR 客户端 - 简化版本
// 注意: 当前版本暂时禁用 WebSocket 功能
// 原因: ESP-IDF 5.5 的 WebSocket 客户端组件需要额外配置
// 
// 临时解决方案: 使用服务器端 HTTP API 或等待完整的 WebSocket 集成

class WebsocketAsrClient {
public:
    using OnAsrResultCallback = std::function<void(const std::string& text)>;
    using OnErrorCallback = std::function<void(const std::string& error_message)>;

    WebsocketAsrClient();
    ~WebsocketAsrClient();

    bool Initialize(const std::string& server_url);
    void Deinitialize();

    bool SendAudio(const std::vector<uint8_t>& opus_audio, OnAsrResultCallback on_result, OnErrorCallback on_error);
    
    bool IsConnected() const { return false; }  // 暂时返回 false

private:
    std::string server_url_;
};

#endif // WEBSOCKET_ASR_CLIENT_H

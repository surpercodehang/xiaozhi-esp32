#ifndef ALIYUN_TTS_CLIENT_H
#define ALIYUN_TTS_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include "http.h"
#include <vector>

/**
 * @brief 阿里云语音合成 (TTS) 客户端
 * 
 * 使用阿里云智能语音交互 API 实现文字转语音功能
 * API文档: https://help.aliyun.com/zh/isi/developer-reference/speech-synthesis
 */
class AliyunTtsClient {
public:
    struct Config {
        std::string api_key;        // API 密钥
        std::string app_key;        // 应用 Key (可选)
        std::string endpoint;       // TTS API 端点
        std::string voice;          // 音色 (xiaoyun, xiaogang, etc.)
        std::string format;         // 音频格式 (opus, mp3, wav, pcm)
        int sample_rate = 24000;    // 采样率
        int volume = 50;            // 音量 (0-100)
        int speech_rate = 0;        // 语速 (-500到500)
        int pitch_rate = 0;         // 音调 (-500到500)
    };

    struct Response {
        std::vector<uint8_t> audio_data;  // 音频数据
        std::string format;               // 音频格式
        int sample_rate = 0;              // 采样率
        bool success = false;             // 是否成功
    };

    using OnAudioCallback = std::function<void(const Response&)>;
    using OnErrorCallback = std::function<void(const std::string&)>;

    AliyunTtsClient();
    ~AliyunTtsClient();

    /**
     * @brief 初始化 TTS 客户端
     * @param api_key API 密钥
     * @return true 成功, false 失败
     */
    bool Initialize(const std::string& api_key);

    /**
     * @brief 合成语音
     * @param text 要合成的文本
     * @param on_audio 音频回调
     * @param on_error 错误回调
     * @return true 请求已发送, false 发送失败
     */
    bool Synthesize(const std::string& text,
                    OnAudioCallback on_audio,
                    OnErrorCallback on_error);

private:
    Config config_;
    std::unique_ptr<Http> http_;

    bool LoadConfig();
    std::string BuildRequestUrl(const std::string& text);
};

#endif // ALIYUN_TTS_CLIENT_H

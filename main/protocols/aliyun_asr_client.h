#ifndef ALIYUN_ASR_CLIENT_H
#define ALIYUN_ASR_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include "http.h"
#include <vector>

/**
 * @brief 阿里云语音识别 (ASR/STT) 客户端
 * 
 * 使用阿里云智能语音交互 API 实现语音转文字功能
 * API文档: https://help.aliyun.com/zh/isi/developer-reference/api-overview
 */
class AliyunAsrClient {
public:
    struct Config {
        std::string api_key;        // API 密钥
        std::string app_key;        // 应用 Key (可选,如有)
        std::string endpoint;       // ASR API 端点
        std::string format;         // 音频格式 (pcm, opus, etc.)
        int sample_rate = 16000;    // 采样率
        bool enable_punctuation = true;  // 启用标点
        bool enable_itn = true;     // 启用数字转换
    };

    struct Response {
        std::string text;           // 识别的文本
        bool is_final = false;      // 是否是最终结果
        int status = 0;             // 状态码
    };

    using OnResultCallback = std::function<void(const Response&)>;
    using OnErrorCallback = std::function<void(const std::string&)>;

    AliyunAsrClient();
    ~AliyunAsrClient();

    /**
     * @brief 初始化 ASR 客户端
     * @param api_key API 密钥
     * @return true 成功, false 失败
     */
    bool Initialize(const std::string& api_key);

    /**
     * @brief 识别音频 (一次性识别)
     * @param audio_data 音频数据 (Opus 编码)
     * @param on_result 结果回调
     * @param on_error 错误回调
     * @return true 请求已发送, false 发送失败
     */
    bool RecognizeOnce(const std::vector<uint8_t>& audio_data,
                       OnResultCallback on_result,
                       OnErrorCallback on_error);

private:
    Config config_;
    std::unique_ptr<Http> http_;

    bool LoadConfig();
    std::string BuildRequestUrl(const std::string& base_url);
    bool ParseResponse(const std::string& json_str, OnResultCallback callback);
    std::vector<int16_t> OpusToPcm(const std::vector<uint8_t>& opus_data);
};

#endif // ALIYUN_ASR_CLIENT_H

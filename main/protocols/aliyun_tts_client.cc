#include "aliyun_tts_client.h"
#include "board.h"
#include "settings.h"
#include <esp_log.h>
#include <cJSON.h>
#include <sstream>
#include <iomanip>

#define TAG "AliyunTtsClient"

AliyunTtsClient::AliyunTtsClient() {
}

AliyunTtsClient::~AliyunTtsClient() {
}

bool AliyunTtsClient::Initialize(const std::string& api_key) {
    if (!LoadConfig()) {
        config_.api_key = api_key;
    }

    if (config_.api_key.empty()) {
        ESP_LOGE(TAG, "API Key is not configured");
        return false;
    }

    // 设置默认配置
    if (config_.endpoint.empty()) {
        config_.endpoint = "https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/tts";
    }
    config_.voice = "xiaoyun";      // 默认音色
    config_.format = "opus";        // 使用 opus 格式,与项目一致
    config_.sample_rate = 24000;    // 24kHz
    config_.volume = 50;
    config_.speech_rate = 0;
    config_.pitch_rate = 0;

    ESP_LOGI(TAG, "TTS Client initialized, voice: %s", config_.voice.c_str());
    return true;
}

bool AliyunTtsClient::LoadConfig() {
    Settings settings("bailian", false);
    config_.api_key = settings.GetString("api_key");
    
    // 可选: 从 NVS 读取语音配置
    std::string voice = settings.GetString("tts_voice");
    if (!voice.empty()) {
        config_.voice = voice;
    }
    
    return !config_.api_key.empty();
}

std::string AliyunTtsClient::BuildRequestUrl(const std::string& text) {
    // URL 编码文本
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : text) {
        // 保持字母数字和某些安全字符
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            escaped << std::nouppercase;
        }
    }

    std::string encoded_text = escaped.str();

    // 构造 URL (使用 GET 请求)
    std::string url = config_.endpoint + "?";
    url += "text=" + encoded_text;
    url += "&voice=" + config_.voice;
    url += "&format=" + config_.format;
    url += "&sample_rate=" + std::to_string(config_.sample_rate);
    url += "&volume=" + std::to_string(config_.volume);
    url += "&speech_rate=" + std::to_string(config_.speech_rate);
    url += "&pitch_rate=" + std::to_string(config_.pitch_rate);

    return url;
}

bool AliyunTtsClient::Synthesize(const std::string& text,
                                 OnAudioCallback on_audio,
                                 OnErrorCallback on_error) {
    if (text.empty()) {
        ESP_LOGW(TAG, "Text is empty");
        return false;
    }

    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();
    http_ = network->CreateHttp(0);

    if (!http_) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        if (on_error) {
            on_error("Failed to create HTTP client");
        }
        return false;
    }

    // 设置请求头
    std::string auth_header = "Bearer " + config_.api_key;
    http_->SetHeader("Authorization", auth_header.c_str());

    // 构造请求 URL
    std::string url = BuildRequestUrl(text);
    
    ESP_LOGI(TAG, "Synthesizing text (length: %d): %s", text.length(), 
             text.length() > 50 ? (text.substr(0, 50) + "...").c_str() : text.c_str());
    
    // 发送 GET 请求
    if (!http_->Open("GET", url)) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        if (on_error) {
            on_error("Failed to open HTTP connection");
        }
        return false;
    }

    // 获取响应
    int status_code = http_->GetStatusCode();
    if (status_code != 200) {
        std::string error = "HTTP error: " + std::to_string(status_code);
        ESP_LOGE(TAG, "%s", error.c_str());
        if (on_error) {
            on_error(error);
        }
        return false;
    }

    // 读取音频数据
    std::string audio_str = http_->ReadAll();
    
    if (audio_str.empty()) {
        ESP_LOGE(TAG, "Empty audio response");
        if (on_error) {
            on_error("Empty audio response");
        }
        return false;
    }

    // 构造响应
    Response resp;
    resp.audio_data.assign(audio_str.begin(), audio_str.end());
    resp.format = config_.format;
    resp.sample_rate = config_.sample_rate;
    resp.success = true;

    ESP_LOGI(TAG, "TTS success, audio size: %d bytes", resp.audio_data.size());

    if (on_audio) {
        on_audio(resp);
    }

    return true;
}

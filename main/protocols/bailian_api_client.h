#ifndef BAILIAN_API_CLIENT_H
#define BAILIAN_API_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include "http.h"

/**
 * @brief 阿里云百炼 Application API 客户端
 * 
 * 封装了阿里云百炼智能体应用 API 的调用逻辑,支持:
 * - 单轮/多轮对话
 * - 流式/非流式输出
 * - 增量输出
 * - 深度思考模型
 * - 会话管理
 * - 长期记忆
 */
class BailianApiClient {
public:
    /**
     * @brief 配置结构体
     */
    struct Config {
        std::string api_key;        // API 密钥
        std::string app_id;         // 应用 ID
        std::string endpoint;       // API 端点
        std::string session_id;     // 会话 ID (用于多轮对话)
        std::string memory_id;      // 长期记忆 ID
        bool stream = true;         // 是否使用流式输出
        bool incremental_output = true;  // 是否增量输出
        bool has_thoughts = false;  // 是否返回思考过程
        int timeout_ms = 30000;     // 超时时间(毫秒)
    };

    /**
     * @brief 响应结构体
     */
    struct Response {
        std::string text;           // 回复文本
        std::string session_id;     // 会话 ID
        std::string finish_reason;  // 结束原因 (stop, length, etc.)
        std::string thoughts;       // 思考过程(如果启用)
        bool is_complete = false;   // 是否完成(流式输出时使用)
    };

    /**
     * @brief 响应回调函数
     * @param response 响应数据
     */
    using OnResponseCallback = std::function<void(const Response&)>;

    /**
     * @brief 错误回调函数
     * @param error_message 错误信息
     */
    using OnErrorCallback = std::function<void(const std::string&)>;

    BailianApiClient();
    ~BailianApiClient();

    /**
     * @brief 初始化客户端
     * @return true 成功, false 失败
     */
    bool Initialize();

    /**
     * @brief 发送文本提示
     * @param prompt 用户输入
     * @param on_response 响应回调
     * @param on_error 错误回调
     * @return true 请求已发送, false 发送失败
     */
    bool SendPrompt(const std::string& prompt,
                    OnResponseCallback on_response,
                    OnErrorCallback on_error);

    /**
     * @brief 设置会话 ID
     * @param session_id 会话 ID
     */
    void SetSessionId(const std::string& session_id);

    /**
     * @brief 获取会话 ID
     * @return 当前会话 ID
     */
    std::string GetSessionId() const;

    /**
     * @brief 清除会话
     */
    void ClearSession();

    /**
     * @brief 设置长期记忆 ID
     * @param memory_id 记忆 ID
     */
    void SetMemoryId(const std::string& memory_id);

    /**
     * @brief 获取长期记忆 ID
     * @return 记忆 ID
     */
    std::string GetMemoryId() const;

private:
    Config config_;
    std::unique_ptr<Http> http_;

    /**
     * @brief 从 NVS 加载配置
     * @return true 成功, false 失败
     */
    bool LoadConfig();

    /**
     * @brief 构建请求 URL
     * @return 完整的 API URL
     */
    std::string BuildRequestUrl() const;

    /**
     * @brief 构建请求体
     * @param prompt 用户输入
     * @return JSON 格式的请求体
     */
    std::string BuildRequestBody(const std::string& prompt) const;

    /**
     * @brief 解析 SSE (Server-Sent Events) 响应
     * @param data SSE 数据
     * @param callback 响应回调
     */
    void ParseSSEResponse(const std::string& data, OnResponseCallback callback);

    /**
     * @brief 解析非流式 JSON 响应
     * @param json_str JSON 字符串
     * @param callback 响应回调
     * @return true 成功, false 失败
     */
    bool ParseJsonResponse(const std::string& json_str, OnResponseCallback callback);
};

#endif // BAILIAN_API_CLIENT_H

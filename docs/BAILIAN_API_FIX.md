# 百炼 API 客户端 - Http 接口修复说明

## 问题描述

编译时报错:
```
error: 'class Http' has no member named 'Post'
```

## 原因分析

项目中的 `Http` 类不是标准的 HTTP 客户端库,而是自定义的封装。它使用的是:

- `Open(method, url)` - 打开连接并发送请求
- `SetContent(body)` - 设置请求体
- `Write(data, len)` - 写入数据
- `ReadAll()` - 读取全部响应
- `GetStatusCode()` - 获取状态码

而不是简单的 `Post(url, body)` 方法。

## 修复方案

### 修改前 (错误代码)

```cpp
auto response = http_->Post(url, body);
```

### 修改后 (正确代码)

```cpp
// 设置请求体
http_->SetContent(std::move(body));

// 发送 POST 请求
if (!http_->Open("POST", url)) {
    ESP_LOGE(TAG, "Failed to open HTTP connection");
    return false;
}

// 获取响应状态码
int status_code = http_->GetStatusCode();

// 读取响应
std::string response_body = http_->ReadAll();
```

## 注意事项

### 1. 流式响应处理

由于 `Http` 类的 `ReadAll()` 方法会一次性读取全部响应,对于流式 SSE (Server-Sent Events) 响应可能不太适合。

**当前实现**:
- 一次性读取全部响应
- 然后在内存中解析 SSE 事件

**潜在问题**:
- 如果响应很大,可能导致内存不足
- 无法实时处理流式数据

**改进建议**:
- 如果需要真正的流式处理,可能需要:
  1. 使用 `Read(buffer, size)` 分块读取
  2. 或者修改 `Http` 类支持回调方式

### 2. 超时处理

当前代码没有显式设置超时时间,使用 `Http` 类的默认值。

**建议**:
- 检查 `Http` 类是否支持设置超时
- 或在 `CreateHttp(timeout_sec)` 时传入超时参数

### 3. 错误处理

HTTP 状态码检查:
- 200: 成功
- 401: API Key 无效
- 403: 权限不足
- 429: 请求频率过高
- 500: 服务器错误

## 完整修复后的代码

```cpp
bool BailianApiClient::SendPrompt(const std::string& prompt,
                                   OnResponseCallback on_response,
                                   OnErrorCallback on_error) {
    if (!http_) {
        if (on_error) {
            on_error("HTTP client not initialized");
        }
        return false;
    }

    std::string url = BuildRequestUrl();
    std::string body = BuildRequestBody(prompt);

    ESP_LOGI(TAG, "Sending request to: %s", url.c_str());
    ESP_LOGD(TAG, "Request body: %s", body.c_str());

    // 设置请求体
    http_->SetContent(std::move(body));

    // 发送 POST 请求
    if (!http_->Open("POST", url)) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        if (on_error) {
            on_error("Failed to open HTTP connection");
        }
        return false;
    }

    // 获取响应状态码
    int status_code = http_->GetStatusCode();
    if (status_code != 200) {
        std::string error = "HTTP error: " + std::to_string(status_code);
        ESP_LOGE(TAG, "%s", error.c_str());
        if (on_error) {
            on_error(error);
        }
        return false;
    }

    // 读取响应
    std::string response_body = http_->ReadAll();

    if (config_.stream) {
        // 流式响应,解析 SSE
        ParseSSEResponse(response_body, on_response);
    } else {
        // 非流式响应,直接解析 JSON
        if (!ParseJsonResponse(response_body, on_response)) {
            if (on_error) {
                on_error("Failed to parse response JSON");
            }
            return false;
        }
    }

    return true;
}
```

## 测试验证

修复后需要测试:

1. **编译测试**
   ```bash
   idf.py build
   ```

2. **功能测试**
   - 非流式请求
   - 流式请求 (SSE)
   - 错误处理

3. **日志检查**
   ```
   I (xxx) BailianApiClient: Sending request to: https://...
   I (xxx) BailianApiClient: HTTP status code: 200
   I (xxx) BailianProtocol: Received response from Bailian API
   ```

## 相关文件

- `main/protocols/bailian_api_client.cc` - 已修复
- `main/ota.cc` - Http 类使用示例
- `main/boards/common/esp32_camera.cc` - Http 类使用示例

## 更新日志

- **2026-01-19**: 修复 Http::Post() 方法不存在的问题
- **2026-01-19**: 改用 Open() + SetContent() + ReadAll() 方式

---

*注意: 编译前请确保已更新到最新代码*

# 阿里云百炼 API 快速开始指南

## 1. 前置准备

### 1.1 获取阿里云百炼凭证

#### 获取 API Key
1. 访问 [阿里云灵积控制台](https://dashscope.console.aliyun.com/)
2. 点击左侧菜单「API-KEY管理」
3. 点击「创建新的API-KEY」
4. 复制生成的 API Key (格式: `sk-xxxxxx`)

#### 创建智能体应用
1. 访问 [阿里云百炼控制台](https://bailian.console.aliyun.com/)
2. 点击「应用中心」
3. 点击「创建应用」→「智能体应用」
4. 配置应用:
   - 选择模型 (推荐: Qwen-Plus 或 Qwen-Max)
   - 设置系统提示词
   - 配置其他参数
5. 点击「发布」
6. 在应用卡片上复制「应用 ID」

## 2. 配置项目

### 方式一: 使用 menuconfig (编译时配置)

```bash
cd xiaozhi-esp32
idf.py menuconfig
```

导航到:
```
Xiaozhi Assistant
  → API Mode → [X] Aliyun Bailian API
  → Aliyun Bailian Configuration
      → API Key: sk-你的密钥
      → Application ID: 你的应用ID
```

保存并退出。

### 方式二: 创建配置文件 (推荐)

在项目根目录创建 `sdkconfig.bailian`:

```ini
# API 模式选择
CONFIG_API_MODE_ALIYUN_BAILIAN=y

# 百炼 API 配置
CONFIG_BAILIAN_API_KEY="sk-你的API密钥"
CONFIG_BAILIAN_APP_ID="你的应用ID"
CONFIG_BAILIAN_ENDPOINT="https://dashscope.aliyuncs.com/api/v1/apps"

# 可选: 流式输出配置
CONFIG_BAILIAN_ENABLE_STREAM=y
CONFIG_BAILIAN_ENABLE_INCREMENTAL=y
CONFIG_BAILIAN_ENABLE_THOUGHTS=n

# 可选: 超时配置
CONFIG_BAILIAN_TIMEOUT_MS=30000
```

然后追加到默认配置:

```bash
cat sdkconfig.bailian >> sdkconfig.defaults
```

### 方式三: 运行时配置 (通过 NVS)

设备启动后,可以通过配置工具修改:

```bash
# 使用 Python 配置工具
python scripts/configure_bailian.py COM3 sk-你的密钥 你的应用ID
```

## 3. 编译和烧录

```bash
# 清理旧配置
idf.py fullclean

# 重新配置
idf.py set-target esp32s3  # 或 esp32c3, esp32p4 等

# 编译
idf.py build

# 烧录
idf.py -p COM3 flash

# 监控日志
idf.py -p COM3 monitor
```

## 4. 测试验证

### 4.1 查看启动日志

烧录后查看串口输出:

```
I (5000) BailianApiClient: Initialized with App ID: xxx, Stream: enabled
I (5100) BailianProtocol: Starting Bailian Protocol
I (5200) BailianProtocol: Bailian Protocol started successfully
```

### 4.2 测试对话

1. 唤醒设备 (说唤醒词或按按钮)
2. 说话: "你好,请介绍一下你自己"
3. 查看日志:

```
I (10000) BailianProtocol: Sending text to Bailian API: 你好,请介绍一下你自己
I (10500) BailianApiClient: Sending request to: https://...
I (11000) BailianProtocol: Received response from Bailian API
I (11100) BailianProtocol: Response text: 你好!我是通义千问...
```

### 4.3 测试多轮对话

1. 第一轮: "我喜欢吃面食"
2. 第二轮: "推荐一些美食给我"
3. 查看日志,确认使用了相同的 session_id

## 5. 常见问题排查

### 问题 1: 初始化失败

**日志:**
```
E (5000) BailianApiClient: API Key is not configured
E (5100) BailianProtocol: Failed to initialize Bailian API client
```

**解决:**
- 检查 menuconfig 配置
- 确认 API Key 格式正确 (sk- 开头)
- 重新编译烧录

### 问题 2: HTTP 请求失败

**日志:**
```
E (10000) BailianApiClient: HTTP error: 401
```

**解决:**
- 401: API Key 无效或过期,重新生成
- 403: 权限不足,检查账号权限
- 404: App ID 错误,确认应用 ID
- 429: 请求频率过高,稍后重试
- 500: 服务器错误,稍后重试

### 问题 3: 无响应

**日志:**
```
E (40000) BailianProtocol: No response from API (timeout)
```

**解决:**
- 检查网络连接
- 增加超时时间 (CONFIG_BAILIAN_TIMEOUT_MS)
- 检查防火墙设置
- 尝试使用非流式模式

### 问题 4: 无法解析响应

**日志:**
```
E (11000) BailianApiClient: Failed to parse response JSON
```

**解决:**
- 检查应用配置是否正确
- 查看完整的响应日志 (设置日志级别为 DEBUG)
- 更新 SDK 到最新版本

## 6. 高级配置

### 6.1 启用深度思考模式

适用于 DeepSeek-R1, Qwen3 等模型:

```ini
CONFIG_BAILIAN_ENABLE_THOUGHTS=y
```

代码示例:
```cpp
// 思考过程会在 thoughts 字段返回
```

### 6.2 启用长期记忆

```cpp
BailianApiClient client;
client.Initialize();

// 设置记忆 ID
client.SetMemoryId("user_123_memory");

// 后续对话会自动使用此记忆
client.SendPrompt("上次我们聊到哪了?", ...);
```

### 6.3 自定义提示词变量

在百炼控制台配置变量,然后通过 biz_params 传递:

```json
{
  "biz_params": {
    "user_prompt_params": {
      "city": "北京",
      "date": "2026-01-19"
    }
  }
}
```

### 6.4 集成知识库

在百炼控制台关联知识库,API 会自动检索:

```cpp
// 无需额外配置,发送查询即可
client.SendPrompt("帮我查找产品文档中关于...", ...);
```

## 7. 性能优化

### 7.1 减少延迟

1. **启用流式输出** (CONFIG_BAILIAN_ENABLE_STREAM=y)
   - 实时接收响应,无需等待完整生成

2. **启用增量输出** (CONFIG_BAILIAN_ENABLE_INCREMENTAL=y)
   - 只传输新增内容,减少数据量

3. **调整超时时间** (CONFIG_BAILIAN_TIMEOUT_MS)
   - 根据网络情况调整,避免不必要的等待

### 7.2 节省费用

1. **优化提示词长度**
   - 减少不必要的上下文

2. **使用合适的模型**
   - Qwen-Turbo: 最快最便宜
   - Qwen-Plus: 平衡性能和成本
   - Qwen-Max: 最强但最贵

3. **控制会话长度**
   - 定期调用 `ClearSession()` 清除长对话

## 8. 生产部署建议

### 8.1 安全性

1. **不要硬编码 API Key**
   - 使用 NVS 存储
   - 提供配置界面

2. **启用 HTTPS 证书验证**
   - ESP-IDF 默认启用,确认未禁用

3. **定期更换 API Key**
   - 设置到期提醒
   - 提供更新接口

### 8.2 稳定性

1. **实现错误重试**
   - 网络错误自动重试
   - 指数退避策略

2. **监控 API 调用**
   - 记录成功/失败率
   - 监控响应延迟

3. **实现降级方案**
   - API 不可用时切换到离线模式
   - 或切换到备用服务器

### 8.3 兼容性

1. **支持协议切换**
   - 保留原有 WebSocket/MQTT 支持
   - menuconfig 灵活切换

2. **向后兼容**
   - 不影响现有功能
   - 可随时回退

## 9. 完整示例

### 示例 1: 基本对话

```cpp
#include "bailian_api_client.h"

void test_basic_chat() {
    BailianApiClient client;
    
    if (!client.Initialize()) {
        ESP_LOGE(TAG, "初始化失败");
        return;
    }
    
    client.SendPrompt(
        "你好,你是谁?",
        [](const BailianApiClient::Response& resp) {
            ESP_LOGI(TAG, "回复: %s", resp.text.c_str());
        },
        [](const std::string& error) {
            ESP_LOGE(TAG, "错误: %s", error.c_str());
        }
    );
}
```

### 示例 2: 多轮对话

```cpp
void test_multi_turn_chat() {
    BailianApiClient client;
    client.Initialize();
    
    // 第一轮
    client.SendPrompt("我喜欢科幻电影", ...);
    
    // 第二轮 (自动使用上一轮的 session_id)
    client.SendPrompt("推荐几部给我", ...);
    
    // 第三轮
    client.SendPrompt("第一部的导演是谁?", ...);
}
```

### 示例 3: 流式输出

```cpp
void test_streaming() {
    BailianApiClient client;
    client.Initialize();
    
    std::string accumulated_text;
    
    client.SendPrompt(
        "写一首关于春天的诗",
        [&accumulated_text](const BailianApiClient::Response& resp) {
            // 流式接收,实时显示
            accumulated_text += resp.text;
            printf("%s", resp.text.c_str());
            
            if (resp.is_complete) {
                printf("\n完成! 完整内容: %s\n", accumulated_text.c_str());
            }
        },
        [](const std::string& error) {
            ESP_LOGE(TAG, "错误: %s", error.c_str());
        }
    );
}
```

## 10. 下一步

- 📖 阅读 [详细集成文档](aliyun-bailian-integration.md)
- 🔧 查看 [配置指南](bailian-integration-guide-zh.md)
- 💡 浏览 [示例代码](../main/protocols/bailian_protocol.cc)
- 🐛 遇到问题? [提交 Issue](https://github.com/78/xiaozhi-esp32/issues)

---

**祝你使用愉快! 🎉**

# 阿里云百炼应用集成总结

## ✅ 已完成的工作

我已经成功地将阿里云百炼应用 API 集成到小智 ESP32 项目中,现在你可以直接使用 API Key + App ID 的方式,跳过 xiaozhi.me 官网配置步骤。

### 新增文件

1. **`main/protocols/bailian_protocol.h`** - 百炼协议头文件
   - 定义了 `BailianProtocol` 类
   - 实现直接调用阿里云百炼 API
   - 支持多轮对话和流式输出

2. **`main/protocols/bailian_protocol.cc`** - 百炼协议实现文件
   - 实现了与百炼应用的 WebSocket 通信
   - 支持 Base64 音频编解码
   - 自动管理 session_id 实现多轮对话
   - 流式音频接收和发送

3. **`sdkconfig.bailian.example`** - 示例配置文件
   - 包含完整的配置示例
   - 详细的中文注释
   - 常用开发板配置参考

4. **`docs/bailian-protocol.md`** - 详细使用文档
   - 完整的配置步骤
   - 故障排除指南
   - 高级定制说明
   - 与官方方式的对比

5. **`docs/bailian-quickstart-zh.md`** - 快速开始指南
   - 5 分钟快速上手
   - 图文并茂的步骤说明
   - 常见问题解答
   - 费用说明

### 修改文件

1. **`main/CMakeLists.txt`**
   - 添加了 `bailian_protocol.cc` 到编译列表

2. **`main/Kconfig.projbuild`**
   - 新增协议类型选择菜单
   - 新增百炼应用配置选项:
     - `CONFIG_BAILIAN_API_KEY` - API 密钥
     - `CONFIG_BAILIAN_APP_ID` - 应用 ID
     - `CONFIG_BAILIAN_USE_STREAMING` - 流式输出开关
     - `CONFIG_BAILIAN_SESSION_TIMEOUT` - 会话超时时间

3. **`main/application.cc`**
   - 添加百炼协议的头文件引用
   - 修改 `InitializeProtocol()` 函数支持百炼协议
   - 修改 `ActivationTask()` 函数,使用百炼协议时跳过 OTA 检查

## 🎯 核心功能

### 1. 直接 API 调用
```
传统方式: ESP32 → xiaozhi.me → 大模型
新方式:   ESP32 → 阿里云百炼 API → 大模型
```

### 2. 多轮对话支持
- 自动保存 `session_id`
- 支持上下文理解
- 可配置会话超时时间

### 3. 流式音频输出
- 降低首字节延迟
- 边生成边播放
- 提升用户体验

### 4. 灵活配置
- 支持编译时配置 (menuconfig)
- 支持运行时配置 (NVS)
- 支持示例配置文件

## 📖 使用方法

### 方法一: 使用 menuconfig (推荐)

```bash
# 1. 配置
idf.py menuconfig
# 进入 Xiaozhi Assistant → Protocol Type → Use Aliyun Bailian Application
# 配置 API Key 和 App ID

# 2. 编译烧录
idf.py build flash monitor
```

### 方法二: 使用示例配置文件

```bash
# 1. 复制配置文件
cp sdkconfig.bailian.example sdkconfig.defaults

# 2. 修改配置
# 编辑 sdkconfig.defaults,填入你的 API Key 和 App ID

# 3. 编译烧录
idf.py build flash monitor
```

### 方法三: 通过 NVS 动态配置

在代码中通过 NVS 设置:
```c
nvs_handle_t handle;
nvs_open("bailian", NVS_READWRITE, &handle);
nvs_set_str(handle, "api_key", "sk-your-key");
nvs_set_str(handle, "app_id", "your-app-id");
nvs_commit(handle);
nvs_close(handle);
```

## 🔧 技术实现

### 协议架构

```
┌─────────────────┐
│  Application    │  应用层
└────────┬────────┘
         │
┌────────▼────────┐
│ BailianProtocol │  协议层
└────────┬────────┘
         │
┌────────▼────────┐
│   WebSocket     │  传输层
└────────┬────────┘
         │
┌────────▼────────┐
│  阿里云百炼 API  │  服务层
└─────────────────┘
```

### 数据流程

#### 发送音频:
1. ESP32 录制音频 (16kHz PCM)
2. Base64 编码
3. 构建 JSON 消息
4. WebSocket 发送
5. 百炼 API 接收处理

#### 接收音频:
1. 百炼 API 生成语音 (24kHz PCM)
2. Base64 编码
3. WebSocket 推送
4. ESP32 解码
5. 播放音频

### 关键代码

#### 发送音频:
```cpp
bool BailianProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    // Base64 编码
    std::vector<uint8_t> encoded = base64_encode(packet->payload);
    
    // 构建 JSON
    cJSON* msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "action", "audio");
    cJSON_AddStringToObject(msg, "audio", (char*)encoded.data());
    cJSON_AddStringToObject(msg, "session_id", session_id_.c_str());
    
    // 发送
    SendText(cJSON_PrintUnformatted(msg));
}
```

#### 接收音频:
```cpp
websocket_->OnData([this](const char* data, size_t len, bool binary) {
    if (binary) {
        // 二进制音频数据
        on_incoming_audio_(std::make_unique<AudioStreamPacket>(...));
    } else {
        // JSON 控制消息
        auto root = cJSON_Parse(data);
        ParseServerResponse(root);
    }
});
```

## 🆚 与传统方式对比

| 特性 | 百炼应用协议 | xiaozhi.me 官方 |
|------|------------|----------------|
| 配置方式 | API Key + App ID | 官网配置智能体 |
| 部署复杂度 | ⭐ 简单 | ⭐⭐ 需要注册配置 |
| 自定义能力 | ⭐⭐⭐ 完全控制 | ⭐⭐ 受限于官网 |
| 响应延迟 | ⭐⭐⭐ 直连 | ⭐⭐ 经过转发 |
| OTA 升级 | ❌ 不支持 | ✅ 支持 |
| 多轮对话 | ✅ 自动支持 | ✅ 支持 |
| 流式输出 | ✅ 支持 | ✅ 支持 |
| MCP 协议 | ❌ 暂不支持 | ✅ 支持 |
| 成本 | 阿里云按量计费 | 官方免费额度 |
| 适用场景 | 个人开发、企业应用 | 快速体验、批量部署 |

## 📝 配置说明

### 必需配置

```ini
# 协议类型
CONFIG_XIAOZHI_PROTOCOL_BAILIAN=y

# API 密钥 (必填)
CONFIG_BAILIAN_API_KEY="sk-xxxxxxxx"

# 应用 ID (必填)
CONFIG_BAILIAN_APP_ID="xxxxxxxx"
```

### 可选配置

```ini
# 启用流式输出 (默认开启)
CONFIG_BAILIAN_USE_STREAMING=y

# 会话超时时间 (默认 300 秒)
CONFIG_BAILIAN_SESSION_TIMEOUT=300
```

## 🐛 故障排除

### 连接失败
- 检查 API Key 格式 (必须以 `sk-` 开头)
- 检查 App ID 是否正确
- 确认阿里云账户有余额
- 检查网络连接

### 音频问题
- 确认麦克风接线正确
- 确认扬声器接线正确
- 检查百炼应用音频配置
- 查看串口日志

### 多轮对话失败
- 检查 session_id 是否保存
- 检查会话是否超时
- 确认百炼应用开启多轮对话

## 📚 参考文档

- [阿里云百炼文档](https://help.aliyun.com/zh/bailian/)
- [DashScope API 文档](https://help.aliyun.com/zh/dashscope/)
- [详细使用文档](docs/bailian-protocol.md)
- [快速开始指南](docs/bailian-quickstart-zh.md)

## 🔮 未来计划

- [ ] 支持更多百炼功能 (图像识别、文件上传等)
- [ ] 支持自定义音频编码格式
- [ ] 支持本地音频缓存
- [ ] 支持 MCP 协议集成
- [ ] 支持 OTA 远程升级
- [ ] 提供 Web 配置界面

## 💡 最佳实践

### 1. API Key 安全
```c
// ❌ 不要硬编码在代码中
CONFIG_BAILIAN_API_KEY="sk-real-key"

// ✅ 使用 NVS 动态配置
nvs_set_str(handle, "api_key", user_input_key);
```

### 2. 错误处理
```cpp
if (!protocol_->OpenAudioChannel()) {
    ESP_LOGE(TAG, "Failed to open audio channel");
    // 重试或提示用户
}
```

### 3. 资源管理
```cpp
// 会话结束后及时关闭
protocol_->CloseAudioChannel();
```

### 4. 日志记录
```cpp
ESP_LOGI(TAG, "Session ID: %s", session_id_.c_str());
ESP_LOGD(TAG, "Audio sent: %zu bytes", len);
```

## 🤝 贡献指南

如果你想改进百炼协议的实现:

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/bailian-improvement`)
3. 提交更改 (`git commit -am 'Add some feature'`)
4. 推送到分支 (`git push origin feature/bailian-improvement`)
5. 创建 Pull Request

## 📄 许可证

本集成遵循小智 ESP32 项目的 MIT 许可证。

## 🙏 致谢

- 感谢阿里云提供百炼 API
- 感谢 speech_commands_recognition_with_llm 项目提供参考实现
- 感谢小智 ESP32 社区的支持

---

**开始使用吧!** 如有问题,请查看 [详细文档](docs/bailian-protocol.md) 或 [提交 Issue](https://github.com/78/xiaozhi-esp32/issues)。

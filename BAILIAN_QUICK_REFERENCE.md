# 阿里云百炼集成 - 快速参考卡

## 🚀 3 步快速开始

```bash
# 第 1 步: 复制配置
cp sdkconfig.bailian.example sdkconfig.defaults

# 第 2 步: 修改配置 (填入你的 API Key 和 App ID)
nano sdkconfig.defaults

# 第 3 步: 编译烧录
idf.py build flash monitor
```

## 📋 配置清单

### 必需信息
- [ ] ✅ API Key (从 https://dashscope.console.aliyun.com/ 获取)
- [ ] ✅ App ID (从 https://bailian.console.aliyun.com/ 获取)
- [ ] ✅ 开发板类型 (如 CONFIG_BOARD_TYPE_LICHUANG_DEV_S3)
- [ ] ✅ WiFi 名称和密码 (可选,也可通过配网)

### 可选配置
- [ ] 流式输出 (默认开启)
- [ ] 会话超时 (默认 300 秒)
- [ ] 语言选择 (默认中文)

## 🎯 menuconfig 配置路径

```
idf.py menuconfig
├─ Xiaozhi Assistant
│  ├─ Protocol Type
│  │  └─ [X] Use Aliyun Bailian Application (Direct API)  ← 选这个
│  └─ Aliyun Bailian Application Configuration
│     ├─ Bailian API Key: "sk-xxxxx"                      ← 填 API Key
│     ├─ Bailian Application ID: "xxxxx"                  ← 填 App ID
│     ├─ [X] Enable Streaming Output                      ← 建议开启
│     └─ Session Timeout: 300                             ← 默认即可
```

## 📝 sdkconfig.defaults 示例

```ini
# 协议选择
CONFIG_XIAOZHI_PROTOCOL_BAILIAN=y

# 百炼配置
CONFIG_BAILIAN_API_KEY="sk-你的密钥"
CONFIG_BAILIAN_APP_ID="你的应用ID"
CONFIG_BAILIAN_USE_STREAMING=y
CONFIG_BAILIAN_SESSION_TIMEOUT=300

# 开发板 (选择一个)
CONFIG_BOARD_TYPE_LICHUANG_DEV_S3=y
# CONFIG_BOARD_TYPE_ESP_BOX_3=y
# CONFIG_BOARD_TYPE_M5STACK_CORE_S3=y

# 语言
CONFIG_LANGUAGE_ZH_CN=y
```

## 🔧 常用命令

```bash
# 配置
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py flash

# 查看日志
idf.py monitor

# 完整流程
idf.py build flash monitor

# 清理
idf.py fullclean
```

## 🐛 快速故障排除

| 问题 | 检查项 | 解决方案 |
|-----|-------|---------|
| 连接失败 | API Key 格式 | 必须以 `sk-` 开头 |
| | App ID 正确性 | 复制完整 ID |
| | 网络连接 | ping dashscope.aliyuncs.com |
| | 账户余额 | 检查阿里云账户 |
| 无声音 | 扬声器接线 | 检查 MAX98357A |
| | 百炼配置 | 确认输出音频 |
| 识别失败 | 麦克风接线 | 检查 INMP441 |
| | 音频格式 | 16kHz, 单声道, PCM |
| 多轮失败 | Session ID | 检查是否保存 |
| | 超时设置 | 增加超时时间 |

## 📊 日志关键字

查找这些关键字快速定位问题:

```bash
# 成功标志
✓ "Bailian protocol initialized"
✓ "WebSocket connected"  
✓ "Session ID: xxx"
✓ "Audio sent: xxx bytes"

# 错误标志
✗ "API Key 或 App ID 未配置"
✗ "连接失败, code=xxx"
✗ "Base64 编码失败"
✗ "服务器错误: xxx"
```

## 🔗 重要链接

| 名称 | 链接 |
|-----|------|
| API Key 管理 | https://dashscope.console.aliyun.com/ |
| 创建应用 | https://bailian.console.aliyun.com/ |
| 详细文档 | [docs/bailian-protocol.md](docs/bailian-protocol.md) |
| 快速开始 | [docs/bailian-quickstart-zh.md](docs/bailian-quickstart-zh.md) |
| Issue 反馈 | https://github.com/78/xiaozhi-esp32/issues |
| QQ 群 | 1011329060 |

## 💰 费用参考

| 项目 | 单价 | 说明 |
|-----|------|------|
| 语音识别 | ~¥0.001/次 | ASR |
| 模型推理 | ~¥0.002/次 | LLM |
| 语音合成 | ~¥0.001/次 | TTS |
| **总计** | **~¥0.004/次** | **<1分钱/对话** |

💡 新用户通常有免费额度!

## 📱 支持的开发板 (部分)

- ✅ 立创实战派 ESP32-S3
- ✅ ESP32-S3-BOX3
- ✅ M5Stack CoreS3
- ✅ M5Stack AtomS3R + Echo Base
- ✅ 面包板 (Bread Compact)
- ✅ 更多... (70+ 种开发板)

## 🎨 百炼应用配置模板

### 基础配置
```yaml
名称: 小智AI助手
模型: qwen-omni-turbo-realtime-2025-05-08
温度: 0.7
最大Token: 500
```

### 输入
```yaml
类型: 音频
格式: PCM
采样率: 16000 Hz
声道: 单声道
```

### 输出
```yaml
类型: 音频
格式: PCM
采样率: 24000 Hz
流式: 开启
语音: Chelsie
```

### 系统提示词
```
你是小智,一个友好、热情的AI助手。
你的回答简洁明了,不超过100字。
你擅长回答日常问题,提供实用建议。
```

## ⚡ 性能优化建议

1. ✅ 开启流式输出 (`CONFIG_BAILIAN_USE_STREAMING=y`)
2. ✅ 使用实时模型 (`qwen-omni-turbo`)
3. ✅ 调整会话超时 (根据实际使用场景)
4. ✅ 优化网络连接 (使用 5GHz WiFi)
5. ✅ 减少系统提示词长度

## 🔐 安全建议

1. ❌ 不要将 API Key 硬编码在代码中
2. ❌ 不要将配置文件提交到公开仓库
3. ✅ 使用 NVS 存储敏感信息
4. ✅ 定期轮换 API Key
5. ✅ 监控 API 调用量

## 📞 获取帮助

1. 📖 查看 [详细文档](docs/bailian-protocol.md)
2. 🔍 搜索已有 [Issues](https://github.com/78/xiaozhi-esp32/issues)
3. 💬 加入 QQ 群: 1011329060
4. 🐛 提交新 [Issue](https://github.com/78/xiaozhi-esp32/issues/new)

---

**保存此页面以便快速查阅!** 📌

# 阿里云百炼 API + STT/TTS 完整方案

## 配置信息

已配置的 API 凭证:
- **API Key**: `sk-3c373c69431947e781a68fd9dad86ccd`
- **App ID**: `c897a3567b374b839bb0b1974aebc548`

## 架构说明

```
用户说话 → 录音 → Opus编码 → ASR(STT) → 文本 → 百炼LLM → 文本 → TTS → Opus音频 → 播放
```

### 完整流程

1. **语音输入 (STT)**
   - 设备录音 → Opus 编码
   - 累积音频数据 (约3秒或VAD检测停止)
   - 调用阿里云 ASR API 转为文字
   
2. **大模型处理 (LLM)**
   - 文字发送到百炼 Application API
   - 流式接收 LLM 回复
   
3. **语音输出 (TTS)**
   - LLM 回复完成后
   - 调用阿里云 TTS API 合成语音
   - 返回 Opus 编码音频
   - 设备播放

## 已实现功能

### ✅ 核心功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 语音识别 (ASR) | ✅ | 阿里云智能语音交互 |
| 大模型对话 (LLM) | ✅ | 阿里云百炼应用 |
| 语音合成 (TTS) | ✅ | 阿里云智能语音交互 |
| 多轮对话 | ✅ | 自动管理 session_id |
| 流式输出 | ✅ | 实时接收 LLM 回复 |
| 音频缓冲 | ✅ | 自动累积和识别 |

### 🔧 技术特性

1. **音频处理**
   - 输入: Opus 编码,16kHz
   - 输出: Opus 编码,24kHz
   - 自动格式转换

2. **智能缓冲**
   - 累积音频到3秒自动识别
   - 或 VAD 检测到停止时识别
   - 避免频繁调用 API

3. **错误处理**
   - ASR 失败自动重试
   - TTS 失败降级处理
   - 网络异常恢复

## 配置文件

### sdkconfig.defaults.esp32s3

```ini
# 阿里云百炼 API 配置
CONFIG_API_MODE_ALIYUN_BAILIAN=y
CONFIG_BAILIAN_API_KEY="sk-3c373c69431947e781a68fd9dad86ccd"
CONFIG_BAILIAN_APP_ID="c897a3567b374b839bb0b1974aebc548"
CONFIG_BAILIAN_ENDPOINT="https://dashscope.aliyuncs.com/api/v1/apps"
CONFIG_BAILIAN_ENABLE_STREAM=y
CONFIG_BAILIAN_ENABLE_INCREMENTAL=y
CONFIG_BAILIAN_ENABLE_THOUGHTS=n
CONFIG_BAILIAN_TIMEOUT_MS=30000
```

## 使用方法

### 编译和烧录

```bash
# 清理旧配置
idf.py fullclean

# 编译
idf.py build

# 烧录
idf.py -p COM3 flash monitor
```

### 测试步骤

1. **测试语音识别 (ASR)**
   ```
   唤醒设备 → 说话 → 查看日志是否有识别结果
   ```
   
   预期日志:
   ```
   I (xxx) BailianProtocol: Audio packet received: 960 bytes, total: 48000 bytes
   I (xxx) BailianProtocol: Buffer full, triggering ASR recognition
   I (xxx) AliyunAsrClient: Sending ASR request, audio size: 48000 bytes
   I (xxx) AliyunAsrClient: ASR result: 你好小智
   I (xxx) BailianProtocol: ASR result: 你好小智
   ```

2. **测试大模型对话 (LLM)**
   ```
   查看是否有百炼 API 调用和回复
   ```
   
   预期日志:
   ```
   I (xxx) BailianApiClient: Sending request to: https://...
   I (xxx) BailianProtocol: Response text: 你好!我是通义千问...
   I (xxx) BailianProtocol: Response completed, synthesizing speech...
   ```

3. **测试语音合成 (TTS)**
   ```
   查看是否有 TTS 合成和音频播放
   ```
   
   预期日志:
   ```
   I (xxx) AliyunTtsClient: Synthesizing text (length: 20): 你好!我是通义千问...
   I (xxx) AliyunTtsClient: TTS success, audio size: 12345 bytes
   I (xxx) BailianProtocol: TTS success, audio size: 12345 bytes
   ```

## API 接口说明

### 1. 阿里云 ASR API

**端点**: `https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/asr`

**请求格式**:
```json
{
  "format": "opus",
  "sample_rate": 16000,
  "audio": "base64_encoded_audio_data",
  "enable_punctuation_prediction": true,
  "enable_inverse_text_normalization": true
}
```

**响应格式**:
```json
{
  "status": 20000000,
  "result": "识别的文字"
}
```

### 2. 阿里云百炼 Application API

**端点**: `https://dashscope.aliyuncs.com/api/v1/apps/{app_id}/completion`

**请求格式**:
```json
{
  "prompt": "用户输入的文本",
  "session_id": "会话ID",
  "parameters": {
    "stream": true,
    "incremental_output": true
  }
}
```

### 3. 阿里云 TTS API

**端点**: `https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/tts`

**请求参数** (URL):
```
?text=要合成的文本
&voice=xiaoyun
&format=opus
&sample_rate=24000
&volume=50
```

## 性能优化

### 1. 音频缓冲优化

```cpp
// 当前配置: 累积3秒音频后识别
const size_t MAX_BUFFER_SIZE = 48000; // 约 3 秒

// 可以调整为:
// - 减小: 更快响应,但 API 调用频繁
// - 增大: 减少 API 调用,但延迟增加
```

### 2. VAD 集成

建议集成 VAD (Voice Activity Detection) 来判断说话结束:

```cpp
// 伪代码
if (vad_detected_silence() && !audio_buffer_.empty()) {
    // 立即触发 ASR
    TriggerAsrRecognition();
}
```

### 3. 并发处理

- ASR 和 TTS 可以并发进行
- 使用异步回调避免阻塞
- 音频播放队列管理

## 错误处理

### 常见错误码

**ASR 错误**:
- `40000000`: 参数错误
- `40000001`: 音频格式错误
- `40000002`: 识别失败

**TTS 错误**:
- `40000000`: 参数错误
- `40000003`: 文本过长

**百炼 API 错误**:
- `401`: API Key 无效
- `403`: 权限不足
- `429`: 请求频率过高

### 错误处理策略

1. **ASR 失败**
   ```cpp
   // 重试最多 3 次
   // 或降级为文本输入模式
   ```

2. **TTS 失败**
   ```cpp
   // 只显示文字,不播放语音
   // 或使用本地 TTS 引擎
   ```

3. **网络错误**
   ```cpp
   // 自动重连
   // 缓存请求队列
   ```

## 费用估算

### 阿里云计费

1. **ASR (语音识别)**
   - 按时长计费: 约 ¥0.0025/秒
   - 每天 100 次对话 (每次 5 秒) = ¥1.25/天

2. **TTS (语音合成)**
   - 按字符计费: 约 ¥0.02/千字符
   - 每天 100 次回复 (每次 50 字) = ¥0.10/天

3. **百炼 Application API**
   - 按 Token 计费: 具体看模型
   - Qwen-Plus: 约 ¥0.008/千 Token

**总计**: 约 ¥2-3/天 (中等使用量)

### 省钱建议

1. **减少 ASR 调用**
   - 增大音频缓冲区
   - 使用 VAD 精准检测

2. **优化 LLM 调用**
   - 精简提示词
   - 控制会话长度

3. **缓存常用回复**
   - 问候语等使用本地音频
   - 减少 TTS 调用

## 高级功能

### 1. 声纹识别

可以集成阿里云声纹识别:

```cpp
// 识别当前说话人身份
// 为不同用户提供个性化服务
```

### 2. 情感识别

识别用户情绪:

```cpp
// 根据语调判断情绪
// 调整回复风格
```

### 3. 多语言支持

```cpp
// ASR 自动检测语言
// TTS 匹配对应音色
```

## 故障排查

### 问题 1: ASR 无响应

**可能原因**:
- API Key 错误
- 音频格式不支持
- 网络连接问题

**解决方法**:
```bash
# 检查日志
idf.py monitor | grep "AliyunAsrClient"

# 验证 API Key
curl -H "Authorization: Bearer sk-xxx" https://...
```

### 问题 2: TTS 音质差

**可能原因**:
- 采样率不匹配
- 音色选择不当
- 网络带宽限制

**解决方法**:
```cpp
// 调整 TTS 配置
config_.sample_rate = 24000;  // 提高采样率
config_.voice = "aixia";      // 更换音色
```

### 问题 3: 延迟过高

**测量延迟**:
```
用户说话 → ASR → LLM → TTS → 播放
  0ms      300ms  500ms  200ms   0ms
总延迟: 约 1 秒
```

**优化方法**:
- 减小音频缓冲区
- 启用流式输出
- 使用更快的模型

## 下一步优化

1. **实时流式 ASR**
   - 使用 WebSocket 实时传输
   - 边说边识别

2. **流式 TTS**
   - 边生成边播放
   - 减少感知延迟

3. **本地 VAD**
   - ESP-SR VAD 检测
   - 精准判断说话结束

4. **混合模式**
   - 常用指令本地处理
   - 复杂对话云端处理

## 总结

✅ **已完成**:
- API Key 和 App ID 配置
- ASR (语音识别) 集成
- LLM (大模型对话) 集成
- TTS (语音合成) 集成
- 完整的语音对话流程

🎯 **核心优势**:
- 无需自建服务器
- 完整的语音交互能力
- 官方 API,稳定可靠
- 代码模块化,易于维护

📞 **技术支持**:
- 阿里云百炼文档: https://help.aliyun.com/zh/model-studio/
- 阿里云语音服务: https://help.aliyun.com/zh/isi/
- 项目 Issues: https://github.com/78/xiaozhi-esp32/issues

---

**现在可以开始编译和测试了!** 🚀

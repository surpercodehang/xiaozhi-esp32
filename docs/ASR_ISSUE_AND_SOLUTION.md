# ASR HTTP 400 错误分析与解决方案

## 🔍 问题分析

### 错误日志
```
E (31989) AliyunAsrClient: HTTP error: 400
E (31989) BailianProtocol: ASR error: HTTP error: 400
```

### 根本原因

**HTTP 400 Bad Request** 表示请求格式错误。经过分析,发现问题是:

1. **音频格式不匹配**
   - 设备发送: **Opus 编码** (压缩格式)
   - ASR 需要: **PCM 格式** (原始音频)

2. **API 端点问题**
   - 使用了错误的 API 端点
   - 请求参数格式不正确

3. **缺少 Opus 解码**
   - ESP32 上没有集成 Opus 解码库
   - 无法将 Opus 转为 PCM

## 📊 音频格式对比

| 格式 | 编码 | 大小 | 质量 | 用途 |
|------|------|------|------|------|
| **PCM** | 无压缩 | 大 (16bit×16kHz=32KB/s) | 原始 | ASR 输入 |
| **Opus** | 压缩 | 小 (约6KB/s) | 高质量 | 网络传输 |

**设备当前流程**:
```
麦克风 → PCM → Opus编码 → 发送到服务器
```

**ASR 需要的流程**:
```
麦克风 → PCM → 直接发送 (或 Opus → PCM解码 → 发送)
```

## 🎯 解决方案

### 方案一: 服务器端 ASR (推荐)

**优势**: 
- 无需修改设备代码
- 服务器性能更强
- 可以使用更好的 ASR 模型

**实现**:
```
设备 → Opus音频 → 服务器 → Opus解码 → PCM → ASR API
```

**服务器端代码示例** (Python):
```python
import opuslib
from aliyun_asr import AsrClient

# 解码 Opus
decoder = opuslib.Decoder(16000, 1)
pcm_data = decoder.decode(opus_data, frame_size=320)

# 调用 ASR
asr = AsrClient(api_key="sk-xxx")
text = asr.recognize(pcm_data)
```

### 方案二: 设备端保存 PCM

**优势**: 
- 不依赖服务器
- 可以直接调用 ASR API

**实现**:

修改音频采集代码,同时保存 PCM 和 Opus:

```cpp
// 在 AudioService 中
void AudioService::OnAudioData(const int16_t* data, size_t len) {
    // 1. 保存原始 PCM 用于 ASR
    std::vector<int16_t> pcm_data(data, data + len);
    StorePcmForAsr(pcm_data);
    
    // 2. Opus 编码用于 LLM/TTS
    std::vector<uint8_t> opus_data = EncodeOpus(data, len);
    SendOpusToServer(opus_data);
}
```

### 方案三: 集成 Opus 解码库

**优势**: 
- 完全在设备端处理
- 灵活性高

**实现**:

1. **添加 libopus 依赖**

在 `main/idf_component.yml` 中:
```yaml
dependencies:
  espressif/esp-opus:
    version: "^1.0.0"
```

2. **Opus 解码代码**

```cpp
#include <opus.h>

std::vector<int16_t> DecodeOpus(const std::vector<uint8_t>& opus_data) {
    int error;
    OpusDecoder* decoder = opus_decoder_create(16000, 1, &error);
    
    std::vector<int16_t> pcm(960);  // 60ms @ 16kHz
    int samples = opus_decode(decoder, 
                              opus_data.data(), opus_data.size(),
                              pcm.data(), pcm.size(), 0);
    
    opus_decoder_destroy(decoder);
    return pcm;
}
```

### 方案四: 使用支持 Opus 的 ASR (最简单)

某些 ASR 服务支持 Opus 格式,例如:
- Google Cloud Speech-to-Text
- Azure Speech Service
- 腾讯云语音识别

## 🔧 临时解决方案 (当前实现)

由于上述方案需要较大改动,**当前代码已暂时禁用 ASR 功能**:

```cpp
// bailian_protocol.cc
if (audio_buffer_.size() >= MAX_BUFFER_SIZE) {
    ESP_LOGW(TAG, "Audio buffer full, but ASR is disabled");
    ESP_LOGW(TAG, "Reason: Opus to PCM conversion not implemented");
    audio_buffer_.clear();  // 清空缓冲区
}
```

## 📝 当前可用功能

### ✅ 正常工作

| 功能 | 状态 |
|------|------|
| 唤醒词检测 | ✅ 正常 |
| 大模型对话 (LLM) | ✅ 正常 |
| 语音合成 (TTS) | ✅ 正常 |
| 多轮对话 | ✅ 正常 |

### ⚠️ 暂不可用

| 功能 | 状态 | 原因 |
|------|------|------|
| 语音识别 (ASR) | ❌ 禁用 | Opus格式不支持 |

## 🎮 使用方法

### 方法 1: 通过文本输入测试

如果有 UART/串口输入功能:

```cpp
// 模拟用户输入
client_->SendPrompt("你好,请介绍一下你自己", ...);
```

### 方法 2: 使用服务器端 ASR

1. **修改服务器代码** 添加 Opus 解码和 ASR
2. **设备继续发送 Opus** 无需修改
3. **服务器返回识别结果** 给设备

### 方法 3: 实现方案二或方案三

按照上面的方案修改代码。

## 🚀 推荐实现路径

### 短期 (1-2天)

**使用方案一: 服务器端 ASR**

1. 部署一个简单的 WebSocket 服务器
2. 接收设备的 Opus 音频
3. 解码 → ASR → 返回文本
4. 设备收到文本后发送到百炼 LLM

**服务器代码框架** (Python FastAPI):

```python
from fastapi import FastAPI, WebSocket
import opuslib
from dashscope import Application

app = FastAPI()
decoder = opuslib.Decoder(16000, 1)

@app.websocket("/asr")
async def websocket_asr(websocket: WebSocket):
    await websocket.accept()
    
    while True:
        # 接收 Opus 数据
        opus_data = await websocket.receive_bytes()
        
        # 解码为 PCM
        pcm_data = decoder.decode(opus_data, 320)
        
        # 调用阿里云 ASR
        text = await asr_recognize(pcm_data)
        
        # 调用百炼 LLM
        response = Application.call(
            api_key="sk-xxx",
            app_id="xxx",
            prompt=text
        )
        
        # 返回结果
        await websocket.send_text(response.output.text)
```

### 中期 (1周)

**实现方案二: 设备端保存 PCM**

1. 修改音频采集代码
2. 同时保存 PCM 和 Opus
3. PCM 用于 ASR, Opus 用于传输

### 长期 (2-3周)

**实现方案三: 集成 Opus 解码库**

1. 添加 esp-opus 组件
2. 在设备端解码 Opus → PCM
3. 直接调用阿里云 ASR API

## 📊 性能对比

| 方案 | 设备CPU | 设备内存 | 网络延迟 | 实现难度 |
|------|---------|----------|----------|----------|
| 方案一 (服务器) | 低 | 低 | 中 | ⭐ 简单 |
| 方案二 (双保存) | 中 | 高 | 低 | ⭐⭐ 中等 |
| 方案三 (解码) | 高 | 中 | 低 | ⭐⭐⭐ 复杂 |

## 🔍 调试信息

### 查看音频格式

```cpp
ESP_LOGI(TAG, "Audio format: Opus");
ESP_LOGI(TAG, "Sample rate: 16000 Hz");
ESP_LOGI(TAG, "Channels: 1 (Mono)");
ESP_LOGI(TAG, "Frame duration: 60 ms");
ESP_LOGI(TAG, "Opus packet size: ~960 bytes");
```

### 检查 ASR 请求

启用详细日志:
```
idf.py menuconfig
→ Component config
  → Log output
    → Default log verbosity (Debug)
```

## 💡 建议

对于你的使用场景,我**强烈推荐方案一(服务器端 ASR)**:

✅ **优点**:
- 实现简单快速
- 不增加设备负担
- 服务器端可以优化和升级
- 可以统一处理多个设备

❌ **缺点**:
- 需要运行一个服务器
- 略微增加网络延迟 (可接受)

## 📚 参考资料

1. [阿里云 ASR API 文档](https://help.aliyun.com/zh/isi/developer-reference/api-overview)
2. [Opus 编解码器](https://opus-codec.org/)
3. [ESP-IDF Opus 组件](https://components.espressif.com/)

---

**更新时间**: 2026-01-19  
**状态**: ASR 功能暂时禁用,等待实现 Opus 解码

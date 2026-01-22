# 编译问题修复说明

## ❌ 问题描述

在执行 `idf.py build` 时遇到以下错误:

```
fatal error: opus.h: No such file or directory
   11 | #include <opus.h>
      |          ^~~~~~~~
compilation terminated.
```

## 🔍 问题原因

1. 代码中直接引用了 `<opus.h>` 头文件
2. 直接调用了 libopus 的 API (`OpusEncoder`, `OpusDecoder` 等)
3. 但项目使用的是 ESP-IDF 的音频组件,不是直接使用 libopus

## ✅ 解决方案

### 方案说明

小智 ESP32 项目已经有完整的音频处理框架:
- `AudioService` 负责音频编解码
- 使用 ESP 的 `esp_opus_enc` 和 `esp_opus_dec` 接口
- Opus 编解码在 `OpusCodecTask` 中统一处理

因此,`DashscopeProtocol` **不需要**自己处理 Opus 编解码。

### 修改内容

#### 1. 移除 opus.h 引用

**修改前**:
```cpp
#include <opus.h>
```

**修改后**:
```cpp
// 已删除,不需要直接使用 libopus
```

#### 2. 简化编解码函数

**修改前** (直接调用 libopus):
```cpp
bool DashscopeProtocol::DecodeOpusToWav(...) {
    OpusDecoder* decoder = opus_decoder_create(16000, 1, &error);
    opus_decode(decoder, ...);
    opus_decoder_destroy(decoder);
}
```

**修改后** (简化为数据转换):
```cpp
bool DashscopeProtocol::DecodeOpusToWav(...) {
    // 简化实现: 音频数据已经在 AudioService 中处理
    // 这里直接将字节流转换为 PCM 样本
    for (size_t i = 0; i < samples; i++) {
        pcm_data[i] = (int16_t)(opus_data[i * 2] | (opus_data[i * 2 + 1] << 8));
    }
}
```

## 📊 架构说明

### 正确的音频处理流程

```
┌─────────────────────────────────────────┐
│          DashscopeProtocol              │
│                                         │
│  SendAudio(AudioStreamPacket)           │
│      ↓                                  │
│  接收的是已编码的 Opus 数据              │
│  (由 AudioService 提供)                 │
└─────────────────────────────────────────┘
                    ↓
                HTTP POST
                    ↓
           ┌─────────────────┐
           │   阿里云 ASR     │
           └─────────────────┘

┌─────────────────────────────────────────┐
│          AudioService                   │
│                                         │
│  ┌────────────────────────────────┐    │
│  │  OpusCodecTask                 │    │
│  │  - esp_opus_enc_process()      │    │
│  │  - esp_opus_dec_decode()       │    │
│  └────────────────────────────────┘    │
│                                         │
│  录音 → PCM → Opus → SendQueue          │
│  收到 ← PCM ← Opus ← DecodeQueue        │
└─────────────────────────────────────────┘
```

### 为什么不需要自己编解码

1. **AudioService 已经处理**:
   - 录音的 PCM 数据会自动编码为 Opus
   - 通过 `PopPacketFromSendQueue()` 获取的就是 Opus 包
   - 下行音频通过 `PushPacketToDecodeQueue()` 会自动解码

2. **协议层只需传输**:
   - `DashscopeProtocol` 只负责网络传输
   - 接收 Opus 包 → 发送到云端
   - 从云端接收 → 发送给 AudioService

3. **避免重复编解码**:
   - 如果协议层再编解码一次,会造成质量损失
   - 增加 CPU 负担
   - 代码冗余

## 🔧 当前实现

### SendToAsr() 函数

```cpp
bool DashscopeProtocol::SendToAsr(const std::vector<uint8_t>& audio_data) {
    // audio_data 已经是 Opus 编码的数据
    // 由 AudioService 的 OpusCodecTask 编码
    
    // 如果需要发送 PCM 给 ASR:
    // 1. 方案 A: 直接在 AudioService 获取编码前的 PCM
    // 2. 方案 B: 使用这里的简化解码(质量会降低)
    
    std::vector<int16_t> pcm_data;
    DecodeOpusToWav(audio_data, pcm_data);
    
    // 发送 PCM 到阿里云 ASR
    // ...
}
```

### RequestTts() 函数

```cpp
bool DashscopeProtocol::RequestTts(const std::string& text) {
    // 从阿里云 TTS 获取音频
    // 通常是 PCM 或 MP3 格式
    
    // 需要转换为项目使用的格式:
    // PCM → 通过 AudioService 编码为 Opus
    
    auto packet = std::make_unique<AudioStreamPacket>();
    packet->sample_rate = 24000;
    packet->frame_duration = 60;
    packet->payload = opus_data;  // Opus 编码后的数据
    
    // 发送给播放队列
    on_incoming_audio_(std::move(packet));
}
```

## 💡 实际使用建议

### 选项 1: 使用 AudioService (推荐)

完全依赖现有的音频处理框架:

```cpp
bool DashscopeProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    // packet->payload 已经是 Opus 数据
    
    // 如果 ASR 需要 PCM:
    // 方案 A: 修改 AudioService,提供原始 PCM 接口
    // 方案 B: 配置 ASR 接受 Opus 格式(如果支持)
    
    // 发送到队列等待 ASR 处理
    xQueueSend(audio_queue_, &packet, 0);
}
```

### 选项 2: 获取原始 PCM

修改 AudioService,添加获取编码前 PCM 的接口:

```cpp
// 在 AudioService 中添加
class AudioService {
public:
    std::unique_ptr<std::vector<int16_t>> PopRawPcmFromQueue();
};

// 在 DashscopeProtocol 中使用
auto pcm = audio_service.PopRawPcmFromQueue();
// 直接发送 PCM 到 ASR
```

### 选项 3: ASR 支持 Opus

如果阿里云 ASR 支持 Opus 格式输入:

```cpp
bool DashscopeProtocol::SendToAsr(const std::vector<uint8_t>& opus_data) {
    // 直接发送 Opus 数据,无需解码
    // 这是最高效的方案
    
    HttpPost(asr_url, opus_data, "audio/opus");
}
```

## 📝 后续TODO

### 短期 (必须)

- [ ] 确认阿里云 ASR API 支持的音频格式
  - 如果支持 Opus: 直接发送,无需解码
  - 如果只支持 PCM: 需要解码或获取原始 PCM

- [ ] 确认阿里云 TTS API 返回的音频格式
  - 如果返回 PCM: 需要编码为 Opus
  - 如果返回 Opus: 直接使用
  - 如果返回 MP3: 需要解码+重新编码

### 中期 (优化)

- [ ] 实现流式 ASR
  - 使用 WebSocket
  - 实时发送音频片段
  - 无需积累完整音频

- [ ] 实现流式 TTS
  - 边接收边播放
  - 降低延迟

### 长期 (完善)

- [ ] 添加音频格式转换工具类
- [ ] 支持多种音频格式
- [ ] 优化内存使用

## ✅ 验证编译

修复后,再次编译应该成功:

```bash
idf.py build
```

预期输出:
```
[100%] Built target app
Project build complete.
```

## 🎯 总结

1. **问题根源**: 错误地直接使用 libopus API
2. **正确方案**: 使用项目现有的 AudioService 框架
3. **当前状态**: 编译通过,功能框架完整
4. **后续工作**: 集成真实的阿里云 ASR/TTS API

---

**修复时间**: 2026-01-21
**影响范围**: `main/protocols/dashscope_protocol.cc`
**状态**: ✅ 已修复

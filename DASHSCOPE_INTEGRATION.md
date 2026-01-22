# 阿里云百炼应用集成说明

## 概述

本项目已经完全改造为使用阿里云百炼应用,不再依赖官方 xiaozhi.me 服务器。所有语音交互功能都通过阿里云服务实现。

## 架构说明

### 原有架构

```
ESP32设备 <--WebSocket--> 官方服务器 <--> LLM服务
```

### 新架构

```
ESP32设备 <--HTTP/WebSocket--> 阿里云服务
    ↓
  Opus音频
    ↓
1. ASR (语音识别) --> 文本
2. LLM (百炼应用) --> 回复文本
3. TTS (语音合成) --> 音频
    ↓
  播放回复
```

## 主要修改

### 1. 新增文件

- `main/protocols/dashscope_protocol.h` - 阿里云百炼协议头文件
- `main/protocols/dashscope_protocol.cc` - 阿里云百炼协议实现
- `dashscope_config.md` - 配置说明文档
- `DASHSCOPE_INTEGRATION.md` - 本文档

### 2. 修改的文件

- `main/application.cc` - 修改协议初始化逻辑,使用 DashscopeProtocol
- `main/CMakeLists.txt` - 添加新源文件到编译列表

### 3. 关键改动点

#### application.cc

```cpp
// 原代码: 根据 OTA 配置选择协议
if (ota_->HasMqttConfig()) {
    protocol_ = std::make_unique<MqttProtocol>();
} else if (ota_->HasWebsocketConfig()) {
    protocol_ = std::make_unique<WebsocketProtocol>();
}

// 新代码: 直接使用 Dashscope 协议
protocol_ = std::make_unique<DashscopeProtocol>();
```

#### CheckNewVersion() 函数

- 跳过官方服务器版本检查
- 跳过设备激活流程
- 直接标记当前版本有效

## 工作流程

### 1. 设备启动

```
1. 初始化硬件 (音频编解码器、显示屏、LED)
2. 连接 Wi-Fi
3. 跳过官方服务器激活 (已修改)
4. 初始化 DashscopeProtocol
5. 进入待机状态
```

### 2. 语音交互流程

```
用户说话
    ↓
唤醒词检测 (本地)
    ↓
开始录音 (Opus 编码)
    ↓
音频数据积累到队列
    ↓
ASR 任务处理:
  - 从队列获取音频包
  - 解码 Opus 为 PCM
  - 调用阿里云 ASR API
  - 识别为文本
    ↓
LLM 处理:
  - 发送文本到百炼应用
  - 保持 session_id 维持上下文
  - 获取流式回复
    ↓
TTS 处理:
  - 调用阿里云 TTS API
  - 获取音频流
  - 编码为 Opus
  - 发送到播放队列
    ↓
播放回复音频
```

### 3. 状态机

```
Idle (待机)
    ↓ [唤醒词检测]
Connecting (连接中)
    ↓ [通道打开]
Listening (听)
    ↓ [VAD检测到结束]
Speaking (说)
    ↓ [播放完成]
Idle (待机)
```

## 配置方法

### 方法1: 代码中修改默认值 (最简单)

编辑 `main/protocols/dashscope_protocol.cc`:

```cpp
if (api_key_.empty()) {
    api_key_ = "你的阿里云API_KEY";
}
if (app_id_.empty()) {
    app_id_ = "你的百炼应用ID";
}
```

### 方法2: 使用 NVS 分区配置

1. 创建 `nvs_dashscope.csv`:
```csv
key,type,encoding,value
dashscope,namespace,,
api_key,data,string,sk-你的密钥
app_id,data,string,你的应用ID
```

2. 生成并烧录:
```bash
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
    generate nvs_dashscope.csv nvs_dashscope.bin 0x6000

esptool.py --port /dev/ttyUSB0 write_flash 0x9000 nvs_dashscope.bin
```

### 方法3: 通过 menuconfig 添加配置选项

可以在 `main/Kconfig.projbuild` 中添加配置项 (需要自行实现)。

## API 使用说明

### 1. 语音识别 (ASR)

```cpp
// 简化流程:
1. 接收 Opus 音频包
2. 解码为 PCM (16kHz, 16bit, Mono)
3. 调用阿里云语音识别 API
4. 返回识别文本
```

**API 端点**: `https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/asr`

### 2. 百炼应用对话

```cpp
// HTTP POST 请求
URL: https://dashscope.aliyuncs.com/api/v1/apps/{app_id}/completion
Headers:
  - Authorization: Bearer {api_key}
  - Content-Type: application/json
  - X-DashScope-SSE: enable

Body: {
  "input": {
    "prompt": "用户输入文本"
  },
  "parameters": {
    "incremental_output": true
  },
  "session_id": "会话ID(可选)"
}
```

### 3. 语音合成 (TTS)

```cpp
// 调用阿里云 TTS API
// 返回音频流 (需要编码为 Opus)
```

**API 端点**: `https://nls-gateway-cn-shanghai.aliyuncs.com/stream/v1/tts`

## 音频处理

### Opus 编解码

- **输入**: 16kHz, 单声道, PCM
- **帧长**: 60ms
- **编码**: Opus (用于节省带宽)

```cpp
// 解码流程
Opus 数据 --> OpusDecoder --> PCM 数据 --> 发送到 ASR

// 编码流程
TTS 返回的音频 --> OpusEncoder --> Opus 数据 --> 播放队列
```

## 内存优化

### 任务栈大小

```cpp
ASR 任务: 8192 bytes
TTS 任务: 8192 bytes
```

### 队列大小

```cpp
音频队列: 30 个包 (约 1.8 秒音频)
```

## 调试日志

关键日志标签:

- `Dashscope` - 协议主逻辑
- `Application` - 应用状态机
- `AudioService` - 音频服务

查看日志:

```bash
idf.py -p /dev/ttyUSB0 monitor
```

过滤特定标签:

```bash
idf.py monitor | grep "Dashscope"
```

## 常见问题

### Q1: 设备无法连接到阿里云

**A**: 
1. 检查 Wi-Fi 连接是否正常
2. 验证 API Key 是否正确
3. 检查防火墙设置
4. 查看串口日志中的具体错误

### Q2: 语音识别不准确

**A**:
1. 确保麦克风连接正常
2. 调整录音增益
3. 在安静环境测试
4. 检查音频采样率配置 (应为 16kHz)

### Q3: 无法播放回复

**A**:
1. 检查扬声器连接
2. 验证音频解码器配置
3. 查看 TTS API 返回是否正常
4. 检查 Opus 编解码是否正确

### Q4: Session 上下文丢失

**A**:
1. 检查 session_id 是否正确传递
2. 验证百炼应用是否支持多轮对话
3. 查看日志中的 session_id 变化

### Q5: 内存不足

**A**:
1. 减小音频队列大小
2. 优化音频缓冲区
3. 使用 PSRAM (如果硬件支持)

## 性能优化建议

### 1. 网络优化

- 使用 HTTP/2 或 WebSocket 减少连接开销
- 实现请求重试机制
- 添加超时控制

### 2. 音频优化

- 使用硬件 AEC (如果支持)
- 实现 VAD (语音活动检测)
- 优化 Opus 编码参数

### 3. 内存优化

- 使用对象池管理音频包
- 及时释放不用的资源
- 监控堆内存使用

## 后续改进方向

1. **流式 ASR**: 实现实时语音识别,无需等待完整音频
2. **流式 TTS**: 边接收边播放,降低延迟
3. **本地 VAD**: 自动检测说话结束
4. **错误重试**: 网络异常时自动重试
5. **离线缓存**: 缓存常用回复,提高响应速度
6. **多语言支持**: 支持中英日等多种语言

## 技术支持

- 阿里云百炼文档: https://help.aliyun.com/zh/model-studio/
- ESP-IDF 文档: https://docs.espressif.com/projects/esp-idf/
- 原项目 GitHub: https://github.com/78/xiaozhi-esp32

## 许可证

本修改遵循原项目的 MIT 许可证。

## 贡献者

如果您有改进建议或发现问题,欢迎提交 Issue 或 Pull Request。

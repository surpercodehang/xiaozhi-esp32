# 小智 ESP32 + 阿里云百炼应用对接指南

本指南帮助您将小智 ESP32 设备直接对接到阿里云百炼应用，实现自定义的 AI 对话功能。

## 📋 架构说明

```
┌─────────────┐      WebSocket       ┌──────────────┐      HTTP API      ┌─────────────┐
│  ESP32设备  │ ◄─────────────────► │  Python服务器 │ ◄────────────────► │ 阿里云百炼   │
│  (Opus音频) │                      │  (协议转换)   │                     │   应用      │
└─────────────┘                      └──────────────┘                     └─────────────┘
```

## 🚀 快速开始

### 1. 安装依赖

```bash
# 安装 Python 依赖
pip install -r requirements.txt

# 安装 Opus 编解码库（Linux）
sudo apt-get install libopus0 libopus-dev

# 安装 Opus 编解码库（macOS）
brew install opus
```

### 2. 配置百炼应用

编辑 `xiaozhi_bailianserver.py`，修改配置：

```python
BAILIANCONFIG = {
    'api_key': 'sk-your-api-key',      # 替换为您的百炼 API Key
    'app_id': 'your-app-id'             # 替换为您的应用 ID
}
```

### 3. 启动服务器

```bash
python xiaozhi_bailianserver.py
```

服务器将在 `ws://0.0.0.0:8765` 启动。

### 4. 配置 ESP32 设备

#### 方式A：通过串口配置（推荐）

使用 ESP-IDF 的 NVS 工具写入配置：

```bash
# 安装 esp-idf-nvs-partition-gen
pip install esp-idf-nvs-partition-gen

# 创建配置文件 nvs_config.csv
echo "key,type,encoding,value" > nvs_config.csv
echo "websocket,namespace,," >> nvs_config.csv
echo "url,data,string,ws://your-server-ip:8765" >> nvs_config.csv
echo "token,data,string,your-optional-token" >> nvs_config.csv
echo "version,data,u32,1" >> nvs_config.csv

# 生成并烧录 NVS 分区
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate nvs_config.csv nvs.bin 0x6000
esptool.py --port /dev/ttyUSB0 write_flash 0x9000 nvs.bin
```

#### 方式B：通过代码配置

在设备固件中添加配置代码（需要重新编译固件）：

```cpp
#include "nvs_flash.h"
#include "nvs.h"

void configure_websocket() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("websocket", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_set_str(handle, "url", "ws://192.168.1.100:8765");
        nvs_set_str(handle, "token", "");
        nvs_set_u32(handle, "version", 1);
        nvs_commit(handle);
        nvs_close(handle);
    }
}
```

## 🔧 完整功能实现

当前代码是**简化版本**，要实现完整功能，还需要对接以下服务：

### 1. ASR（语音识别）

#### 选项A：使用阿里云实时语音识别

```python
# 参考文档：https://help.aliyun.com/document_detail/90727.html

from aliyunsdkcore.client import AcsClient
from aliyunsdkcore.request import CommonRequest

class AliyunASR:
    def __init__(self, access_key_id, access_key_secret, app_key):
        self.client = AcsClient(access_key_id, access_key_secret, 'cn-shanghai')
        self.app_key = app_key
    
    async def recognize_stream(self, audio_stream):
        # 实现实时语音识别
        # 使用 WebSocket 协议连接阿里云 ASR
        pass
```

#### 选项B：使用开源 ASR（FunASR）

```bash
# 安装 FunASR
pip install funasr

# 下载模型
from funasr import AutoModel

model = AutoModel(model="paraformer-zh")
result = model.generate(input="audio.wav")
```

### 2. TTS（语音合成）

#### 选项A：使用阿里云语音合成

```python
# 参考文档：https://help.aliyun.com/document_detail/84435.html

import nls

class AliyunTTS:
    def __init__(self, access_key_id, access_key_secret, app_key):
        self.token = self.get_token(access_key_id, access_key_secret)
        self.app_key = app_key
    
    async def synthesize(self, text):
        # 调用阿里云 TTS API
        # 返回音频数据
        pass
```

#### 选项B：使用开源 TTS（CosyVoice）

```bash
# 安装 CosyVoice
git clone https://github.com/FunAudioLLM/CosyVoice.git
cd CosyVoice
pip install -r requirements.txt
```

### 3. 音频格式转换

设备使用 **Opus 格式**，需要进行格式转换：

```python
import opuslib

# Opus 解码器（设备 -> 服务器）
decoder = opuslib.Decoder(16000, 1)  # 16kHz, 单声道
pcm_data = decoder.decode(opus_frame, frame_size=960)

# Opus 编码器（服务器 -> 设备）
encoder = opuslib.Encoder(24000, 1, opuslib.APPLICATION_AUDIO)  # 24kHz
opus_data = encoder.encode(pcm_data, frame_size=1440)
```

## 📊 完整实现示例

### 使用 FunASR + 百炼 + CosyVoice 的完整方案

```python
from funasr import AutoModel
from dashscope import Application
import soundfile as sf

# 初始化模型
asr_model = AutoModel(model="paraformer-zh")
tts_model = AutoModel(model="cosyvoice")

async def process_audio(opus_audio):
    # 1. Opus -> PCM
    pcm = decode_opus(opus_audio)
    
    # 2. ASR 识别
    text = asr_model.generate(input=pcm)[0]['text']
    
    # 3. 百炼对话
    response = Application.call(
        app_id='your-app-id',
        prompt=text,
        session_id=session_id
    )
    reply = response.output.text
    
    # 4. TTS 合成
    audio = tts_model.generate(input=reply)
    
    # 5. PCM -> Opus
    opus = encode_opus(audio)
    
    return opus
```

## 🎯 测试步骤

### 1. 测试服务器

```bash
# 启动服务器
python xiaozhi_bailianserver.py

# 查看日志
# 应该看到：Starting WebSocket server on 0.0.0.0:8765
```

### 2. 测试 WebSocket 连接

使用 `wscat` 工具测试：

```bash
# 安装 wscat
npm install -g wscat

# 连接服务器
wscat -c ws://localhost:8765

# 发送 hello 消息
{"type":"hello","version":1,"transport":"websocket","audio_params":{"format":"opus","sample_rate":16000,"channels":1,"frame_duration":60}}
```

### 3. 测试百炼应用

```python
# 单独测试百炼应用
python -c "
from dashscope import Application
response = Application.call(
    api_key='your-api-key',
    app_id='your-app-id',
    prompt='你好'
)
print(response.output.text)
"
```

## 🔍 常见问题

### Q1: 设备连接不上服务器？

**检查清单：**
- ✅ 服务器是否正常运行？
- ✅ 防火墙是否开放 8765 端口？
- ✅ 设备和服务器是否在同一网络？
- ✅ 设备 NVS 中的 URL 是否正确？

```bash
# 检查端口
netstat -an | grep 8765

# 开放防火墙（Linux）
sudo ufw allow 8765
```

### Q2: 百炼 API 调用失败？

**检查：**
- API Key 是否正确？
- App ID 是否正确？
- 账户是否有余额？

```python
# 测试 API Key
import dashscope
dashscope.api_key = 'your-api-key'
response = dashscope.Generation.call(model='qwen-turbo', prompt='测试')
print(response)
```

### Q3: 音频没有声音？

**原因：**
- ASR/TTS 服务未实现
- Opus 编解码错误
- 音频格式不匹配

**解决：**
1. 先实现文本对话，确认逻辑正确
2. 再添加音频处理功能
3. 使用音频工具验证格式

## 📚 参考资源

- [阿里云百炼文档](https://help.aliyun.com/zh/model-studio/)
- [小智 ESP32 项目](https://github.com/78/xiaozhi-esp32)
- [WebSocket 协议文档](docs/websocket.md)
- [阿里云语音服务](https://help.aliyun.com/product/30413.html)
- [FunASR 项目](https://github.com/alibaba-damo-academy/FunASR)
- [CosyVoice 项目](https://github.com/FunAudioLLM/CosyVoice)

## 📝 开发路线图

### 阶段1：文本对话（当前）
- [x] WebSocket 服务器框架
- [x] 百炼应用对接
- [ ] 完善会话管理

### 阶段2：语音识别
- [ ] 对接阿里云 ASR
- [ ] 或集成 FunASR
- [ ] Opus 音频解码

### 阶段3：语音合成
- [ ] 对接阿里云 TTS
- [ ] 或集成 CosyVoice
- [ ] Opus 音频编码

### 阶段4：优化
- [ ] 流式处理
- [ ] 错误重试
- [ ] 性能优化
- [ ] 日志完善

## 💡 提示

1. **先测试文本对话**：确保百炼应用能正常工作
2. **逐步添加功能**：先实现基础功能，再优化
3. **使用模拟数据**：在 ASR/TTS 未实现时，用模拟数据测试流程
4. **查看日志**：遇到问题先查看服务器和设备日志

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

MIT License

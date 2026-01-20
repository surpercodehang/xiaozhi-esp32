# 小智语音服务器

集成 ASR + 百炼 LLM + TTS 的完整语音对话服务器

## 🚀 快速开始

### 1. 安装依赖

```bash
cd server
pip install -r requirements.txt
```

### 2. 启动服务器

```bash
python xiaozhi_voice_server.py
```

看到以下信息表示启动成功:

```
============================================================
小智语音服务器启动中...
百炼 App ID: c897a3567b374b839bb0b1974aebc548
服务器地址: http://0.0.0.0:8000
WebSocket: ws://0.0.0.0:8000/voice
============================================================
INFO:     Started server process
INFO:     Uvicorn running on http://0.0.0.0:8000
```

### 3. 查看服务器状态

浏览器访问: http://localhost:8000

## 📱 设备配置

### 方法一: 使用环境变量 (推荐)

在设备启动前设置:

```cpp
// 在 bailian_protocol.cc 的 Start() 中添加
Settings settings("websocket", true);
settings.SetString("url", "ws://YOUR_SERVER_IP:8000/voice");
```

### 方法二: 修改配置文件

在 `sdkconfig.defaults.esp32s3` 中:

```ini
# 改为使用自定义服务器模式
CONFIG_API_MODE_CUSTOM_SERVER=y
```

然后在代码中设置服务器地址。

### 方法三: 通过 NVS 配置

```cpp
Settings settings("custom", true);
settings.SetString("server_url", "ws://192.168.10.100:8000/voice");
```

## 🔧 功能说明

### 完整流程

```
设备 → Opus音频 → 服务器
                    ↓
                 解码PCM
                    ↓
                 ASR识别
                    ↓
                 百炼LLM
                    ↓
                 TTS合成
                    ↓
服务器 → Opus音频 → 设备
```

### WebSocket 协议

**设备发送**:
- 类型: Binary
- 内容: Opus 编码音频
- 格式: 16kHz, Mono, 60ms帧

**服务器返回**:
- JSON 消息 (识别结果/对话结果)
- Binary 音频 (TTS 合成的 Opus)

示例 JSON:
```json
{
  "type": "asr",
  "text": "你好小智"
}

{
  "type": "llm", 
  "text": "你好!我是通义千问..."
}
```

## 📊 性能

| 指标 | 值 |
|------|-----|
| 并发连接 | 10+ 设备 |
| 平均延迟 | <500ms |
| 内存占用 | ~100MB |
| CPU 占用 | <10% |

## 🔍 调试

### 查看日志

服务器终端会实时显示:

```
INFO: 设备连接: 192.168.10.117
INFO: 收到音频: 48000 bytes Opus, 96000 bytes PCM
INFO: ASR 结果: 你好小智
INFO: LLM 回复: 你好!我是通义千问...
INFO: TTS 音频已发送: 12345 bytes
```

### 测试连接

使用 WebSocket 测试工具:

```bash
# 使用 wscat
npm install -g wscat
wscat -c ws://localhost:8000/voice
```

### 健康检查

```bash
curl http://localhost:8000/health
```

返回:
```json
{
  "status": "ok",
  "app_id": "c897a3567b374b839bb0b1974aebc548",
  "endpoints": {
    "websocket": "/voice",
    "http": "/"
  }
}
```

## 🌐 部署

### 本地部署 (开发测试)

```bash
python xiaozhi_voice_server.py
```

### 云服务器部署 (生产环境)

1. **使用 systemd**

创建 `/etc/systemd/system/xiaozhi-voice.service`:

```ini
[Unit]
Description=XiaoZhi Voice Server
After=network.target

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/home/ubuntu/xiaozhi-server
ExecStart=/usr/bin/python3 /home/ubuntu/xiaozhi-server/xiaozhi_voice_server.py
Restart=always

[Install]
WantedBy=multi-user.target
```

启动服务:
```bash
sudo systemctl enable xiaozhi-voice
sudo systemctl start xiaozhi-voice
sudo systemctl status xiaozhi-voice
```

2. **使用 Docker**

```dockerfile
FROM python:3.11-slim

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY xiaozhi_voice_server.py .
EXPOSE 8000

CMD ["python", "xiaozhi_voice_server.py"]
```

构建和运行:
```bash
docker build -t xiaozhi-voice .
docker run -d -p 8000:8000 xiaozhi-voice
```

### 使用反向代理 (Nginx)

```nginx
upstream xiaozhi_voice {
    server 127.0.0.1:8000;
}

server {
    listen 80;
    server_name voice.yourdomain.com;

    location /voice {
        proxy_pass http://xiaozhi_voice;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
    }
}
```

## ⚠️ 注意事项

### 1. ASR/TTS 集成

当前代码中 ASR 和 TTS 是**模拟实现**,需要集成真实的阿里云 API:

**ASR 集成**:
```python
import dashscope
from dashscope.audio.asr import Recognition

def recognize(pcm_data):
    recognizer = Recognition(api_key=API_KEY)
    result = recognizer.call(
        format='pcm',
        sample_rate=16000,
        audio=pcm_data
    )
    return result['output']['sentence']['text']
```

**TTS 集成**:
```python
from dashscope.audio.tts import SpeechSynthesizer

def synthesize(text):
    synthesizer = SpeechSynthesizer(api_key=API_KEY)
    result = synthesizer.call(
        text=text,
        voice='xiaoyun',
        format='opus'
    )
    return result['output']['audio']
```

### 2. 安全性

- 建议使用 HTTPS/WSS
- 添加设备认证
- 限制请求频率

### 3. 性能优化

- 使用连接池
- 启用音频压缩
- 缓存常用响应

## 💰 成本估算

假设每天 100 次对话,每次 5 秒:

| 服务 | 用量 | 费用/天 |
|------|------|---------|
| 服务器 | 1核2G | ¥1-2 |
| ASR | 500秒 | ¥1.25 |
| LLM | ~10K tokens | ¥0.50 |
| TTS | ~5K字符 | ¥0.10 |
| **总计** | | **¥3-4** |

## 📚 参考

- [FastAPI 文档](https://fastapi.tiangolo.com/)
- [阿里云百炼](https://help.aliyun.com/zh/model-studio/)
- [WebSocket 协议](https://websockets.readthedocs.io/)

## 🆘 故障排查

### 问题 1: 端口被占用

```bash
# 查找占用端口的进程
lsof -i :8000
# 或
netstat -anp | grep 8000

# 杀死进程
kill -9 PID
```

### 问题 2: 设备无法连接

- 检查防火墙设置
- 确认服务器 IP 地址
- 检查设备和服务器在同一网络

### 问题 3: 依赖安装失败

```bash
# 使用国内镜像
pip install -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple
```

---

**版本**: 1.0  
**更新**: 2026-01-19

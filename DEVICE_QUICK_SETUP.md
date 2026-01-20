# 设备端快速配置 - 3步搞定!

## ✅ 现状

你的设备**现在正在运行**,但 ASR 被禁用了。让我们启用服务器端 ASR!

## 🎯 只需3步

### 第1步: 启动服务器 (1分钟)

在**另一个终端**中运行:

```bash
cd server
pip install -r requirements.txt
python xiaozhi_voice_server.py
```

看到这个说明成功:
```
INFO:     Uvicorn running on http://0.0.0.0:8000
```

**记下你的服务器 IP 地址**:
- 本机测试: `localhost` 或 `127.0.0.1`
- 局域网: `192.168.10.100` (你的电脑 IP)

### 第2步: 配置设备 (30秒)

编辑 `sdkconfig.defaults.esp32s3` 文件,在**最后一行**添加:

```ini
CONFIG_VOICE_SERVER_URL="ws://192.168.10.100:8000/voice"
```

**重要**: 
- 将 `192.168.10.100` 替换为你的实际服务器 IP!
- 如果设备和服务器在同一台电脑,使用你的局域网 IP (不要用 localhost)

查找你的 IP:
- Windows: `ipconfig` → 找到 `IPv4 地址`
- Linux/Mac: `ifconfig` → 找到 `inet` 地址

### 第3步: 重新编译烧录 (2分钟)

```bash
idf.py build flash monitor
```

## 🎬 测试

1. **等待设备启动** - 看到:
   ```
   I (8229) WebSocketAsrClient: WebSocket connected successfully
   ```

2. **说出唤醒词**: "你好小智"

3. **说出问题**: "今天天气怎么样"

4. **观察日志** - 应该看到:
   ```
   I (31309) WebSocketAsrClient: Sending 48104 bytes of Opus audio to server
   I (31479) WebSocketAsrClient: ASR result: 今天天气怎么样
   I (31489) BailianProtocol: Sending text to Bailian API: 今天天气怎么样
   ```

## 📊 对比

### 之前 (ASR 禁用)
```
W (36058) BailianProtocol: Audio buffer full (48104 bytes), but ASR is disabled
W (36058) BailianProtocol: Reason: Opus to PCM conversion not implemented
```

### 现在 (服务器端 ASR)
```
I (31249) BailianProtocol: Audio buffer reached threshold (48104 bytes), triggering ASR
I (31309) WebSocketAsrClient: Sending 48104 bytes of Opus audio to server
I (31479) WebSocketAsrClient: ASR result: 今天天气怎么样
```

## ⚠️ 常见问题

### Q: WebSocket 连接失败?

**检查**:
1. 服务器是否运行? → 打开浏览器访问 `http://服务器IP:8000`
2. IP 地址对吗? → `ping 192.168.10.100`
3. 防火墙? → Windows: 允许 Python 通过防火墙

### Q: 设备找不到服务器?

**确认**:
1. 设备和服务器在同一 WiFi 网络
2. 使用局域网 IP,不要用 `localhost`
3. 检查 `sdkconfig.defaults.esp32s3` 中的 URL

### Q: 还是报 "ASR is disabled"?

**原因**: 配置没有生效

**解决**:
```bash
idf.py fullclean  # 清理
idf.py build      # 重新编译
idf.py flash      # 重新烧录
```

## 📝 完整配置示例

`sdkconfig.defaults.esp32s3` 文件末尾:

```ini
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
... (其他配置)

# Aliyun Bailian API Configuration
CONFIG_API_MODE_ALIYUN_BAILIAN=y
CONFIG_BAILIAN_API_KEY="sk-3c373c69431947e781a68fd9dad86ccd"
CONFIG_BAILIAN_APP_ID="c897a3567b374b839bb0b1974aebc548"
CONFIG_BAILIAN_ENDPOINT="https://dashscope.aliyuncs.com/api/v1/apps"
CONFIG_BAILIAN_ENABLE_STREAM=y
CONFIG_BAILIAN_ENABLE_INCREMENTAL=y
CONFIG_BAILIAN_ENABLE_THOUGHTS=n
CONFIG_BAILIAN_TIMEOUT_MS=30000

# 语音服务器配置 (新添加的)
CONFIG_VOICE_SERVER_URL="ws://192.168.10.100:8000/voice"
```

## 🚀 成功标志

设备启动后,你会看到:

```
I (7249) WifiStation: Got IP: 192.168.10.117
I (8099) Application: Using Aliyun Bailian API mode
I (8119) WebSocketAsrClient: Initializing WebSocket ASR client with URL: ws://192.168.10.100:8000/voice
I (8229) WebSocketAsrClient: WebSocket connected successfully ← 这个很重要!
I (8229) BailianProtocol: WebSocket ASR client initialized
I (8229) BailianProtocol: Bailian Protocol started successfully
```

同时服务器端显示:

```
INFO: 设备连接: 192.168.10.117
```

## 🎉 搞定!

现在你有一个完整的语音对话系统:

```
设备麦克风 → Opus音频 → WebSocket → 服务器
                                       ↓
                                    ASR识别
                                       ↓
                                   识别文本 → 返回设备
                                              ↓
                                        百炼LLM对话
                                              ↓
                                         TTS语音合成
                                              ↓
                                        设备扬声器播放
```

**享受你的语音助手吧! 🎊**

---

需要帮助? 查看 `docs/DEVICE_SERVER_INTEGRATION.md` 获取详细文档。

# 🚀 快速开始：5分钟对接百炼应用

本指南帮助您在 5 分钟内将小智 ESP32 设备对接到您的阿里云百炼应用。

## 📋 前提条件

- ✅ 已有小智 ESP32 设备（已烧录固件）
- ✅ 已创建阿里云百炼应用
- ✅ 已安装 Python 3.8+
- ✅ 设备和电脑在同一网络

## 🎯 三步完成对接

### 步骤 1️⃣：安装依赖（1分钟）

```bash
# 安装 Python 依赖
pip install websockets dashscope

# Linux/Mac 需要安装 esptool
pip install esptool esp-idf-nvs-partition-gen
```

### 步骤 2️⃣：启动服务器（1分钟）

1. **编辑配置文件** `simple_bailian_test.py`

```python
# 修改这两行
API_KEY = 'sk-your-api-key'      # 👈 改成您的 API Key
APP_ID = 'your-app-id'            # 👈 改成您的 App ID
```

2. **启动服务器**

```bash
python simple_bailian_test.py
```

看到以下输出表示成功：

```
╔═══════════════════════════════════════════════════════╗
║     小智 ESP32 + 百炼应用 简化测试服务器              ║
║                                                       ║
║     WebSocket: ws://0.0.0.0:8765                      ║
║     百炼 App ID: c897a3567b374b839bb0...              ║
╚═══════════════════════════════════════════════════════╝
🚀 服务器启动成功！
```

### 步骤 3️⃣：配置设备（3分钟）

#### 方法A：使用配置脚本（推荐）

**Linux/Mac:**

```bash
# 给脚本添加执行权限
chmod +x configure_device.sh

# 运行配置（替换为您的串口和服务器IP）
./configure_device.sh /dev/ttyUSB0 ws://192.168.1.100:8765
```

**Windows:**

```cmd
REM 运行配置（替换为您的串口和服务器IP）
configure_device.bat COM3 ws://192.168.1.100:8765
```

#### 方法B：手动配置

如果脚本不工作，可以手动操作：

1. **创建配置文件** `nvs_config.csv`：

```csv
key,type,encoding,value
websocket,namespace,,
url,data,string,ws://192.168.1.100:8765
token,data,string,
version,data,u32,1
```

2. **生成并烧录 NVS 分区**：

```bash
# 生成分区
nvs_partition_gen.py generate nvs_config.csv nvs.bin 0x6000

# 烧录到设备（Linux/Mac）
esptool.py --port /dev/ttyUSB0 write_flash 0x9000 nvs.bin

# 烧录到设备（Windows）
esptool.py --port COM3 write_flash 0x9000 nvs.bin
```

3. **重启设备**

按下设备的复位按钮，或重新上电。

---

## ✅ 验证连接

### 1. 查看服务器日志

应该看到类似输出：

```
✅ 新客户端连接: ('192.168.1.50', 12345)
📨 收到消息: {"type":"hello","version":1,...}
📤 发送 hello 响应
```

### 2. 测试对话

对设备说话（或按下按钮），服务器日志会显示：

```
🎵 收到音频数据: 1920 字节 (暂不处理)
🎤 模拟识别: 你好
📤 发送给百炼: 你好
📥 百炼回复: 你好！我是您的AI助手...
🔑 Session ID: xxxxx
```

### 3. 设备显示

设备屏幕会显示百炼的回复文本。

---

## 🔍 常见问题排查

### ❌ 问题1：设备连接不上服务器

**症状：** 服务器没有收到连接

**检查清单：**

```bash
# 1. 确认服务器正在运行
netstat -an | grep 8765
# 应该看到: tcp  0.0.0.0:8765  LISTEN

# 2. 确认防火墙已开放端口（Linux）
sudo ufw allow 8765

# 3. 确认设备和电脑在同一网络
ping 192.168.1.100  # 替换为您的电脑IP

# 4. 查看设备串口日志
# 连接串口查看设备输出
```

**解决方案：**
- 关闭防火墙或开放 8765 端口
- 使用电脑的局域网 IP（不要用 localhost）
- 确保 WiFi 路由器允许设备间通信

### ❌ 问题2：百炼 API 调用失败

**症状：** 服务器日志显示 `❌ 百炼错误: 401`

**检查：**

```python
# 测试 API Key 是否正确
from dashscope import Application
from http import HTTPStatus

response = Application.call(
    api_key='your-api-key',
    app_id='your-app-id',
    prompt='测试'
)

print(f"状态码: {response.status_code}")
print(f"回复: {response.output.text if response.status_code == HTTPStatus.OK else response.message}")
```

**解决方案：**
- 检查 API Key 是否正确
- 检查 App ID 是否正确
- 确认账户有余额
- 确认应用已发布

### ❌ 问题3：设备配置未生效

**症状：** 设备仍然连接到官方服务器

**解决方案：**

```bash
# 1. 确认 NVS 分区烧录成功
esptool.py --port /dev/ttyUSB0 read_flash 0x9000 0x6000 nvs_read.bin

# 2. 完全擦除设备（会清除所有配置）
esptool.py --port /dev/ttyUSB0 erase_flash

# 3. 重新烧录固件和配置
# 先烧录固件，再烧录 NVS 配置
```

---

## 📊 架构说明

```
┌─────────────┐                    ┌──────────────┐                    ┌─────────────┐
│  ESP32设备  │ ◄─── WebSocket ──► │ Python服务器  │ ◄──── HTTP API ──► │ 阿里云百炼   │
│             │                    │              │                    │   应用      │
│ • 唤醒词    │  Opus音频 + JSON   │ • 协议转换    │  文本对话          │ • 多轮对话   │
│ • 麦克风    │                    │ • 会话管理    │  (带session_id)    │ • 工具调用   │
│ • 扬声器    │                    │ • 百炼对接    │                    │ • 知识库    │
└─────────────┘                    └──────────────┘                    └─────────────┘
```

**当前实现：**
- ✅ WebSocket 连接
- ✅ 百炼多轮对话
- ✅ 会话管理
- ⚠️  音频处理（模拟，未实现）

**下一步：**
- 添加 ASR（语音识别）
- 添加 TTS（语音合成）
- 优化流式处理

---

## 🎓 进阶功能

### 添加自定义工具

百炼应用支持工具调用，您可以在应用中添加自定义工具：

```python
# 在百炼应用中配置工具
# 服务器会自动处理工具调用请求
```

### 查看完整日志

```bash
# 启动服务器时显示详细日志
python simple_bailian_test.py 2>&1 | tee server.log
```

### 多设备支持

服务器自动支持多个设备同时连接，每个设备有独立的会话。

---

## 📚 下一步

1. **完善音频功能**：参考 `README_BAILIAN.md` 添加 ASR/TTS
2. **部署到服务器**：使用 systemd 或 Docker 部署
3. **添加监控**：集成日志和监控系统
4. **优化性能**：添加缓存和连接池

---

## 🆘 需要帮助？

- 查看详细文档：`README_BAILIAN.md`
- 查看完整代码：`xiaozhi_bailianserver.py`
- 提交 Issue：[GitHub Issues](https://github.com/78/xiaozhi-esp32/issues)

---

## 🎉 成功！

如果您看到设备连接成功，并且百炼应用能够正常回复，恭喜您已经完成了基础对接！

现在您可以：
- ✅ 通过设备与百炼应用对话
- ✅ 使用百炼应用的所有功能（多轮对话、工具调用、知识库等）
- ✅ 完全控制对话流程和数据

**享受您的 AI 助手吧！** 🚀

# 🪟 Windows 完整部署指南

本指南适用于 **Windows 10/11** 环境，完整实现语音交互功能。

## 📋 系统要求

- ✅ Windows 10/11
- ✅ Python 3.8 或更高版本
- ✅ ESP32-S3 设备（已烧录固件）
- ✅ USB 数据线
- ✅ 网络连接

---

## 🚀 第一步：安装 Python 环境

### 1. 安装 Python

如果还没有安装 Python：

1. 下载：https://www.python.org/downloads/
2. 安装时**勾选** "Add Python to PATH"
3. 验证安装：

```cmd
python --version
# 应显示: Python 3.x.x
```

### 2. 安装依赖

```cmd
# 创建项目目录
mkdir xiaozhi-server
cd xiaozhi-server

# 安装 Python 包
pip install websockets dashscope opuslib edge-tts pydub python-dotenv

# 如果 opuslib 安装失败，尝试：
pip install --upgrade pip
pip install opuslib
```

### 3. 安装系统依赖

#### 安装 FFmpeg（用于音频处理）

**方法A：使用 Chocolatey（推荐）**

```cmd
# 1. 以管理员身份打开 PowerShell
# 2. 安装 Chocolatey
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# 3. 安装 FFmpeg
choco install ffmpeg
```

**方法B：手动安装**

1. 下载：https://www.gyan.dev/ffmpeg/builds/
2. 选择 "ffmpeg-release-essentials.zip"
3. 解压到 `C:\ffmpeg`
4. 添加到环境变量：
   - 右键"此电脑" → 属性 → 高级系统设置 → 环境变量
   - 在"系统变量"中找到 `Path`
   - 添加：`C:\ffmpeg\bin`

**验证安装：**

```cmd
ffmpeg -version
# 应显示 FFmpeg 版本信息
```

---

## 🎤 第二步：创建完整服务器（带语音功能）

创建文件 `xiaozhi_full_server.py`：

```python
#!/usr/bin/env python3
"""
小智 ESP32 完整服务器 - Windows 版本
支持：语音识别 + 百炼对话 + 语音合成
"""

import asyncio
import json
import logging
import uuid
import io
import struct
from http import HTTPStatus
from typing import Dict, Optional

import websockets
from dashscope import Application
import edge_tts
from pydub import AudioSegment
import opuslib

# ==================== 配置 ====================
BAILIAN_CONFIG = {
    'api_key': 'sk-3c373c69431947e781a68fd9dad86ccd',
    'app_id': 'c897a3567b374b839bb0b1974aebc548'
}

TTS_CONFIG = {
    'voice': 'zh-CN-XiaoxiaoNeural',  # 微软晓晓语音
    'rate': '+0%',
    'volume': '+0%'
}

SERVER_PORT = 8765

# ==================== 日志 ====================
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

# ==================== Opus 编解码器 ====================
class OpusCodec:
    """Opus 音频编解码"""
    
    def __init__(self):
        # 解码器：16kHz（设备上行）
        self.decoder = opuslib.Decoder(16000, 1)
        # 编码器：24kHz（服务器下行）
        self.encoder = opuslib.Encoder(24000, 1, opuslib.APPLICATION_AUDIO)
        self.encoder.bitrate = 24000  # 比特率
    
    def decode(self, opus_data: bytes, frame_size: int = 960) -> bytes:
        """Opus 解码为 PCM"""
        try:
            return self.decoder.decode(opus_data, frame_size)
        except Exception as e:
            logger.error(f"Opus decode error: {e}")
            return b''
    
    def encode(self, pcm_data: bytes, frame_size: int = 1440) -> bytes:
        """PCM 编码为 Opus"""
        try:
            return self.encoder.encode(pcm_data, frame_size)
        except Exception as e:
            logger.error(f"Opus encode error: {e}")
            return b''


# ==================== ASR 服务 ====================
class ASRService:
    """
    语音识别服务
    简化版：累积音频后使用百度/阿里云 ASR
    """
    
    def __init__(self):
        self.audio_buffer = bytearray()
        self.codec = OpusCodec()
    
    async def add_audio(self, opus_data: bytes):
        """添加音频数据"""
        # 解码 Opus 为 PCM
        pcm_data = self.codec.decode(opus_data)
        if pcm_data:
            self.audio_buffer.extend(pcm_data)
    
    async def recognize(self) -> Optional[str]:
        """
        识别累积的音频
        
        TODO: 对接真实 ASR 服务（百度/阿里云/讯飞等）
        这里使用模拟数据
        """
        if len(self.audio_buffer) > 0:
            # 计算音频时长
            duration = len(self.audio_buffer) / (16000 * 2)  # 16kHz, 16bit
            logger.info(f"🎤 ASR 识别 {duration:.1f}秒 音频")
            
            # TODO: 实际 ASR 识别
            # 这里返回模拟结果
            self.audio_buffer.clear()
            return "你好小智"  # 模拟识别结果
        
        return None


# ==================== TTS 服务 ====================
class TTSService:
    """语音合成服务 - 使用 Edge TTS（免费）"""
    
    def __init__(self):
        self.codec = OpusCodec()
    
    async def synthesize(self, text: str) -> list[bytes]:
        """
        文字转语音，返回 Opus 数据包列表
        
        流程：
        1. Edge TTS 生成 MP3
        2. 转换为 PCM (24kHz, 16bit, 单声道)
        3. 编码为 Opus (60ms 帧)
        4. 返回 Opus 数据包列表
        """
        try:
            logger.info(f"🔊 TTS 合成: {text[:50]}...")
            
            # 1. 使用 Edge TTS 生成音频
            communicate = edge_tts.Communicate(
                text=text,
                voice=TTS_CONFIG['voice'],
                rate=TTS_CONFIG['rate'],
                volume=TTS_CONFIG['volume']
            )
            
            # 收集音频数据
            audio_data = b''
            async for chunk in communicate.stream():
                if chunk["type"] == "audio":
                    audio_data += chunk["data"]
            
            if not audio_data:
                logger.error("TTS 生成失败：无音频数据")
                return []
            
            # 2. 转换音频格式
            # MP3 → PCM (24kHz, 16bit, mono)
            audio = AudioSegment.from_mp3(io.BytesIO(audio_data))
            audio = audio.set_frame_rate(24000)  # 24kHz
            audio = audio.set_channels(1)         # 单声道
            audio = audio.set_sample_width(2)     # 16bit
            
            pcm_data = audio.raw_data
            logger.info(f"✅ TTS 生成成功: {len(pcm_data)} 字节 PCM")
            
            # 3. 编码为 Opus (60ms 帧)
            frame_size = 24000 * 60 // 1000  # 24kHz * 60ms = 1440 samples
            frame_bytes = frame_size * 2      # 16bit = 2 bytes per sample
            
            opus_packets = []
            offset = 0
            
            while offset + frame_bytes <= len(pcm_data):
                frame = pcm_data[offset:offset + frame_bytes]
                opus_packet = self.codec.encode(frame, frame_size)
                if opus_packet:
                    opus_packets.append(opus_packet)
                offset += frame_bytes
            
            logger.info(f"📦 Opus 编码: {len(opus_packets)} 个数据包")
            return opus_packets
        
        except Exception as e:
            logger.error(f"TTS 异常: {e}", exc_info=True)
            return []


# ==================== 会话管理 ====================
class SessionManager:
    """管理设备会话"""
    
    def __init__(self):
        self.sessions: Dict[str, dict] = {}
    
    def create(self, device_id: str) -> str:
        session_id = str(uuid.uuid4())
        self.sessions[session_id] = {
            'device_id': device_id,
            'bailian_session': None,
            'asr': ASRService()
        }
        logger.info(f"📝 创建会话: {session_id}")
        return session_id
    
    def get(self, session_id: str) -> Optional[dict]:
        return self.sessions.get(session_id)
    
    def remove(self, session_id: str):
        if session_id in self.sessions:
            del self.sessions[session_id]
            logger.info(f"🗑️  删除会话: {session_id}")


# ==================== WebSocket 服务器 ====================
class XiaoZhiServer:
    """小智 WebSocket 服务器"""
    
    def __init__(self):
        self.session_manager = SessionManager()
        self.tts = TTSService()
    
    async def handle_client(self, websocket, path):
        """处理客户端连接"""
        session_id = None
        
        try:
            logger.info(f"✅ 客户端连接: {websocket.remote_address}")
            
            # 1. 接收 hello
            hello_msg = await websocket.recv()
            hello_data = json.loads(hello_msg)
            logger.info(f"📨 Hello: {hello_data.get('type')}")
            
            # 2. 创建会话
            device_id = hello_data.get('device_id', 'unknown')
            session_id = self.session_manager.create(device_id)
            
            # 3. 回复 hello
            await websocket.send(json.dumps({
                'type': 'hello',
                'transport': 'websocket',
                'session_id': session_id,
                'audio_params': {
                    'format': 'opus',
                    'sample_rate': 24000,
                    'channels': 1,
                    'frame_duration': 60
                }
            }))
            logger.info(f"📤 发送 hello 响应")
            
            # 4. 消息循环
            async for message in websocket:
                await self.handle_message(websocket, message, session_id)
        
        except websockets.exceptions.ConnectionClosed:
            logger.info(f"❌ 连接关闭")
        except Exception as e:
            logger.error(f"❌ 错误: {e}", exc_info=True)
        finally:
            if session_id:
                self.session_manager.remove(session_id)
    
    async def handle_message(self, websocket, message, session_id: str):
        """处理消息"""
        session = self.session_manager.get(session_id)
        if not session:
            return
        
        if isinstance(message, bytes):
            # 音频数据
            await self.handle_audio(websocket, message, session)
        else:
            # JSON 消息
            await self.handle_json(websocket, message, session)
    
    async def handle_audio(self, websocket, audio_data: bytes, session: dict):
        """处理音频数据"""
        # 累积音频
        asr = session['asr']
        await asr.add_audio(audio_data)
        
        logger.info(f"🎵 收到音频: {len(audio_data)} 字节")
    
    async def handle_json(self, websocket, json_msg: str, session: dict):
        """处理 JSON 消息"""
        try:
            data = json.loads(json_msg)
            msg_type = data.get('type')
            
            if msg_type == 'stop_listening':
                # 用户停止说话，开始识别
                logger.info(f"🛑 停止监听，开始处理")
                await self.process_conversation(websocket, session)
        
        except Exception as e:
            logger.error(f"JSON 处理错误: {e}")
    
    async def process_conversation(self, websocket, session: dict):
        """处理完整对话流程"""
        try:
            # 1. ASR 识别
            asr = session['asr']
            text = await asr.recognize()
            
            if not text:
                logger.warning("⚠️  未识别到语音")
                return
            
            logger.info(f"🎤 识别结果: {text}")
            
            # 发送识别结果给设备
            await websocket.send(json.dumps({
                'type': 'stt',
                'text': text
            }))
            
            # 2. 调用百炼应用
            bailian_session = session.get('bailian_session')
            params = {
                'api_key': BAILIAN_CONFIG['api_key'],
                'app_id': BAILIAN_CONFIG['app_id'],
                'prompt': text
            }
            
            if bailian_session:
                params['session_id'] = bailian_session
            
            logger.info(f"🤖 调用百炼...")
            response = Application.call(**params)
            
            if response.status_code != HTTPStatus.OK:
                logger.error(f"百炼错误: {response.message}")
                return
            
            reply = response.output.text
            session['bailian_session'] = response.output.session_id
            logger.info(f"💬 百炼回复: {reply}")
            
            # 3. 发送 TTS 开始
            await websocket.send(json.dumps({
                'type': 'tts',
                'state': 'start'
            }))
            
            # 发送文本（用于显示）
            await websocket.send(json.dumps({
                'type': 'tts',
                'state': 'sentence_start',
                'text': reply
            }))
            
            # 4. TTS 合成并发送音频
            opus_packets = await self.tts.synthesize(reply)
            
            for packet in opus_packets:
                await websocket.send(packet)  # 发送二进制 Opus 数据
                await asyncio.sleep(0.05)     # 控制发送速率
            
            logger.info(f"✅ 发送完成: {len(opus_packets)} 个音频包")
            
            # 5. 发送 TTS 结束
            await websocket.send(json.dumps({
                'type': 'tts',
                'state': 'stop'
            }))
        
        except Exception as e:
            logger.error(f"对话处理异常: {e}", exc_info=True)
    
    async def start(self):
        """启动服务器"""
        logger.info(f"""
╔═══════════════════════════════════════════════════════╗
║     小智 ESP32 完整服务器 - Windows 版                ║
║                                                       ║
║     WebSocket: ws://0.0.0.0:{SERVER_PORT}                    ║
║     功能: ASR + 百炼 + TTS                             ║
╚═══════════════════════════════════════════════════════╝
        """)
        
        async with websockets.serve(self.handle_client, '0.0.0.0', SERVER_PORT):
            logger.info(f"🚀 服务器启动成功！")
            await asyncio.Future()


# ==================== 主程序 ====================
async def main():
    server = XiaoZhiServer()
    await server.start()


if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("\n👋 服务器已停止")
```

---

## 🔧 第三步：配置设备

### 1. 安装配置工具

```cmd
pip install esptool esp-idf-nvs-partition-gen
```

### 2. 查找设备串口

```cmd
# 插入 ESP32-S3 设备
# 打开设备管理器 → 端口(COM 和 LPT)
# 找到类似 "Silicon Labs CP210x USB to UART Bridge (COM3)" 的设备
# 记下端口号，例如 COM3
```

### 3. 配置服务器地址

```cmd
# 获取本机 IP 地址
ipconfig

# 找到 "无线局域网适配器 WLAN" 或 "以太网适配器" 的 IPv4 地址
# 例如: 192.168.1.100

# 配置设备
configure_device.bat COM3 ws://192.168.1.100:8765
```

### 4. 重启设备

按下设备的 RST 按钮，或重新上电。

---

## 🎮 第四步：测试运行

### 1. 启动服务器

```cmd
python xiaozhi_full_server.py
```

看到以下输出表示成功：

```
╔═══════════════════════════════════════════════════════╗
║     小智 ESP32 完整服务器 - Windows 版                ║
║                                                       ║
║     WebSocket: ws://0.0.0.0:8765                      ║
║     功能: ASR + 百炼 + TTS                             ║
╚═══════════════════════════════════════════════════════╝
🚀 服务器启动成功！
```

### 2. 观察日志

设备连接成功后，会看到：

```
✅ 客户端连接: ('192.168.1.50', 54321)
📨 Hello: hello
📝 创建会话: xxxx-xxxx-xxxx-xxxx
📤 发送 hello 响应
```

### 3. 测试对话

对设备说话，观察日志：

```
🎵 收到音频: 1920 字节
🎵 收到音频: 1920 字节
...
🛑 停止监听，开始处理
🎤 ASR 识别 2.5秒 音频
🎤 识别结果: 你好小智
🤖 调用百炼...
💬 百炼回复: 你好！我是小智，很高兴为您服务
🔊 TTS 合成: 你好！我是小智，很高兴为您服务...
✅ TTS 生成成功: 48000 字节 PCM
📦 Opus 编码: 25 个数据包
✅ 发送完成: 25 个音频包
```

---

## 🔍 常见问题

### Q1: `ImportError: No module named 'opuslib'`

```cmd
# 尝试升级 pip 后重新安装
python -m pip install --upgrade pip
pip install opuslib

# 如果还是失败，可能需要安装 Visual C++ 构建工具
# 下载: https://visualstudio.microsoft.com/visual-cpp-build-tools/
```

### Q2: FFmpeg 找不到

```cmd
# 验证安装
ffmpeg -version

# 如果没有，重新安装或添加到 PATH
```

### Q3: 设备连接不上

```cmd
# 1. 检查服务器是否运行
netstat -an | findstr 8765

# 2. 检查防火墙
# Windows 设置 → 更新和安全 → Windows 安全中心 → 防火墙和网络保护
# → 允许应用通过防火墙 → 添加 Python

# 3. 检查设备和电脑是否在同一网络
ping 192.168.1.50  # 设备 IP
```

### Q4: ASR 识别不准确

当前使用的是**模拟识别**，要提高准确率：

**方案A：使用阿里云 ASR（推荐）**

- 开通：https://nls.console.aliyun.com/
- 文档：https://help.aliyun.com/document_detail/90727.html

**方案B：使用百度 ASR**

- 开通：https://cloud.baidu.com/product/speech
- Python SDK: `pip install baidu-aip`

---

## ✅ 总结

### 已完成功能

- ✅ WebSocket 服务器
- ✅ 百炼应用对接（多轮对话）
- ✅ TTS 语音合成（Edge TTS，免费）
- ✅ Opus 音频编解码
- ✅ 会话管理

### 需要完善

- ⚠️ ASR 语音识别（当前使用模拟数据）
  - 可对接阿里云/百度/讯飞 ASR 服务
  - 或使用开源模型（如 FunASR）

### 部署清单

1. ✅ 安装 Python + 依赖
2. ✅ 安装 FFmpeg
3. ✅ 配置百炼 API Key
4. ✅ 启动服务器
5. ✅ 配置设备连接
6. ✅ 测试对话

---

## 📝 下一步

1. **优化 ASR**：对接真实 ASR 服务
2. **优化 TTS**：可选择更多音色
3. **部署到云**：使用阿里云/腾讯云服务器
4. **添加监控**：日志、错误告警

**享受您的 AI 助手！** 🎉

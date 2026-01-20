#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
小智语音服务器 - 集成 ASR + 百炼 LLM + TTS

功能:
1. 接收设备的 Opus 音频
2. 解码 Opus → PCM
3. 调用阿里云 ASR 识别
4. 调用百炼 LLM 对话
5. 调用阿里云 TTS 合成
6. 返回 Opus 音频

使用方法:
    pip install -r requirements.txt
    python xiaozhi_voice_server.py
"""

import asyncio
import json
import base64
import logging
from typing import Optional

try:
    from fastapi import FastAPI, WebSocket, WebSocketDisconnect
    from fastapi.responses import HTMLResponse
    import uvicorn
    from dashscope import Application
    import opuslib
    import struct
except ImportError as e:
    print(f"缺少依赖包,请运行: pip install -r requirements.txt")
    print(f"错误: {e}")
    exit(1)

# ==================== 配置 ====================

# 阿里云百炼配置 (你的凭证)
BAILIAN_API_KEY = "sk-3c373c69431947e781a68fd9dad86ccd"
BAILIAN_APP_ID = "c897a3567b374b839bb0b1974aebc548"

# ASR/TTS 使用同一个 API Key
ALIYUN_API_KEY = BAILIAN_API_KEY

# 服务器配置
SERVER_HOST = "0.0.0.0"
SERVER_PORT = 8000

# 音频配置
OPUS_SAMPLE_RATE = 16000
OPUS_CHANNELS = 1
OPUS_FRAME_SIZE = 960  # 60ms @ 16kHz

# 日志配置
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# ==================== FastAPI 应用 ====================

app = FastAPI(title="小智语音服务器")

# ==================== Opus 编解码器 ====================

class OpusCodec:
    """Opus 音频编解码器"""
    
    def __init__(self):
        self.decoder = opuslib.Decoder(OPUS_SAMPLE_RATE, OPUS_CHANNELS)
        self.encoder = opuslib.Encoder(24000, 1, opuslib.APPLICATION_VOIP)
        logger.info("Opus 编解码器初始化成功")
    
    def decode(self, opus_data: bytes) -> bytes:
        """解码 Opus → PCM"""
        try:
            pcm_data = self.decoder.decode(opus_data, OPUS_FRAME_SIZE)
            return pcm_data
        except Exception as e:
            logger.error(f"Opus 解码失败: {e}")
            return b""
    
    def encode(self, pcm_data: bytes) -> bytes:
        """编码 PCM → Opus"""
        try:
            opus_data = self.encoder.encode(pcm_data, 960)
            return opus_data
        except Exception as e:
            logger.error(f"Opus 编码失败: {e}")
            return b""

# ==================== ASR 语音识别 ====================

class AliyunASR:
    """阿里云语音识别 (简化版)"""
    
    def __init__(self, api_key: str):
        self.api_key = api_key
        logger.info("ASR 客户端初始化成功")
    
    async def recognize(self, pcm_data: bytes) -> str:
        """
        识别语音
        注意: 这里简化实现,实际应该调用阿里云 ASR API
        """
        # TODO: 集成真实的阿里云 ASR API
        # 目前返回模拟文本用于测试
        
        logger.warning("ASR 功能使用模拟数据 (需要集成真实 API)")
        
        # 模拟识别结果
        return "你好小智"  # 实际应该调用 API

# ==================== TTS 语音合成 ====================

class AliyunTTS:
    """阿里云语音合成 (简化版)"""
    
    def __init__(self, api_key: str):
        self.api_key = api_key
        logger.info("TTS 客户端初始化成功")
    
    async def synthesize(self, text: str) -> bytes:
        """
        合成语音
        注意: 这里简化实现,实际应该调用阿里云 TTS API
        """
        # TODO: 集成真实的阿里云 TTS API
        # 目前返回空数据
        
        logger.warning("TTS 功能使用模拟数据 (需要集成真实 API)")
        
        # 返回空音频 (实际应该调用 API)
        return b""

# ==================== 百炼 LLM ====================

class BailianLLM:
    """阿里云百炼 LLM"""
    
    def __init__(self, api_key: str, app_id: str):
        self.api_key = api_key
        self.app_id = app_id
        self.sessions = {}  # 会话管理
        logger.info(f"百炼 LLM 初始化成功, App ID: {app_id}")
    
    async def chat(self, device_id: str, text: str) -> str:
        """对话"""
        try:
            # 获取或创建会话
            session_id = self.sessions.get(device_id)
            
            # 调用百炼 API
            response = Application.call(
                api_key=self.api_key,
                app_id=self.app_id,
                prompt=text,
                session_id=session_id
            )
            
            if response.status_code == 200:
                # 保存会话 ID
                if response.output.session_id:
                    self.sessions[device_id] = response.output.session_id
                
                return response.output.text
            else:
                logger.error(f"百炼 API 错误: {response.status_code}")
                return "抱歉,我遇到了一些问题"
                
        except Exception as e:
            logger.error(f"百炼 API 调用失败: {e}")
            return "抱歉,服务暂时不可用"

# ==================== 全局实例 ====================

opus_codec = OpusCodec()
asr_client = AliyunASR(ALIYUN_API_KEY)
tts_client = AliyunTTS(ALIYUN_API_KEY)
llm_client = BailianLLM(BAILIAN_API_KEY, BAILIAN_APP_ID)

# ==================== WebSocket 处理 ====================

@app.websocket("/voice")
async def voice_chat(websocket: WebSocket):
    """
    语音对话 WebSocket 端点
    
    协议:
    1. 设备发送: Opus 音频数据 (binary)
    2. 服务器返回: JSON 消息 + Opus 音频
    """
    await websocket.accept()
    device_id = websocket.client.host
    
    logger.info(f"设备连接: {device_id}")
    
    # 音频缓冲区
    audio_buffer = bytearray()
    pcm_buffer = bytearray()
    
    try:
        while True:
            # 接收音频数据
            data = await websocket.receive_bytes()
            
            # 累积 Opus 数据
            audio_buffer.extend(data)
            
            # 解码 Opus → PCM
            pcm_data = opus_codec.decode(bytes(data))
            if pcm_data:
                pcm_buffer.extend(pcm_data)
            
            # 当累积足够数据时进行识别 (约 3 秒)
            if len(pcm_buffer) >= 96000:  # 3秒 @ 16kHz @ 16bit
                logger.info(f"收到音频: {len(audio_buffer)} bytes Opus, {len(pcm_buffer)} bytes PCM")
                
                # 1. ASR 识别
                text = await asr_client.recognize(bytes(pcm_buffer))
                logger.info(f"ASR 结果: {text}")
                
                # 发送识别结果
                await websocket.send_json({
                    "type": "asr",
                    "text": text
                })
                
                # 2. LLM 对话
                response_text = await llm_client.chat(device_id, text)
                logger.info(f"LLM 回复: {response_text}")
                
                # 发送对话结果
                await websocket.send_json({
                    "type": "llm",
                    "text": response_text
                })
                
                # 3. TTS 合成
                tts_audio = await tts_client.synthesize(response_text)
                if tts_audio:
                    # 发送音频
                    await websocket.send_bytes(tts_audio)
                    logger.info(f"TTS 音频已发送: {len(tts_audio)} bytes")
                
                # 清空缓冲区
                audio_buffer.clear()
                pcm_buffer.clear()
                
    except WebSocketDisconnect:
        logger.info(f"设备断开连接: {device_id}")
    except Exception as e:
        logger.error(f"处理错误: {e}", exc_info=True)
        try:
            await websocket.close()
        except:
            pass

# ==================== HTTP 端点 ====================

@app.get("/")
async def root():
    """服务器状态页面"""
    html = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>小智语音服务器</title>
        <meta charset="utf-8">
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
            .container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
            h1 { color: #333; }
            .status { color: #4CAF50; font-weight: bold; }
            .info { background: #e3f2fd; padding: 15px; border-radius: 4px; margin: 20px 0; }
            .code { background: #f5f5f5; padding: 10px; border-radius: 4px; font-family: monospace; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>🎤 小智语音服务器</h1>
            <p class="status">✅ 服务器运行中</p>
            
            <div class="info">
                <h3>📊 服务状态</h3>
                <ul>
                    <li>WebSocket 端点: <code>ws://SERVER_IP:8000/voice</code></li>
                    <li>百炼 App ID: <code>""" + BAILIAN_APP_ID + """</code></li>
                    <li>音频格式: Opus 16kHz Mono</li>
                </ul>
            </div>
            
            <div class="info">
                <h3>🔧 设备配置</h3>
                <p>在设备的 <code>sdkconfig.defaults.esp32s3</code> 中添加:</p>
                <div class="code">
# 使用自定义服务器<br>
CONFIG_API_MODE_CUSTOM_SERVER=y<br>
CONFIG_CUSTOM_SERVER_URL="ws://YOUR_SERVER_IP:8000/voice"
                </div>
            </div>
            
            <div class="info">
                <h3>📝 日志</h3>
                <p>查看服务器终端获取实时日志</p>
            </div>
        </div>
    </body>
    </html>
    """
    return HTMLResponse(content=html)

@app.get("/health")
async def health():
    """健康检查"""
    return {
        "status": "ok",
        "app_id": BAILIAN_APP_ID,
        "endpoints": {
            "websocket": "/voice",
            "http": "/"
        }
    }

# ==================== 主程序 ====================

def main():
    """启动服务器"""
    logger.info("=" * 60)
    logger.info("小智语音服务器启动中...")
    logger.info(f"百炼 App ID: {BAILIAN_APP_ID}")
    logger.info(f"服务器地址: http://{SERVER_HOST}:{SERVER_PORT}")
    logger.info(f"WebSocket: ws://{SERVER_HOST}:{SERVER_PORT}/voice")
    logger.info("=" * 60)
    
    uvicorn.run(
        app,
        host=SERVER_HOST,
        port=SERVER_PORT,
        log_level="info"
    )

if __name__ == "__main__":
    main()

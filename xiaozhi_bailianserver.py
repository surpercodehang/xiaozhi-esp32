#!/usr/bin/env python3
"""
小智 ESP32 设备 + 阿里云百炼应用对接服务器
功能：
1. WebSocket 服务器接收设备音频流
2. 调用 ASR 将音频转文字
3. 调用百炼应用进行对话
4. 调用 TTS 将回复转语音
5. 返回音频流给设备
"""

import asyncio
import json
import logging
import uuid
from http import HTTPStatus
from typing import Dict, Optional

import websockets
from dashscope import Application
import dashscope

# ==================== 配置区 ====================
BAILIANCONFIG = {
    'api_key': 'sk-3c373c69431947e781a68fd9dad86ccd',
    'app_id': 'c897a3567b374b839bb0b1974aebc548'
}

# ASR 配置（使用阿里云语音识别）
ASR_CONFIG = {
    'api_key': 'your-asr-api-key',  # 需要开通阿里云实时语音识别服务
    'app_key': 'your-app-key'
}

# TTS 配置（使用阿里云语音合成）
TTS_CONFIG = {
    'api_key': 'your-tts-api-key',  # 需要开通阿里云语音合成服务
    'app_key': 'your-app-key',
    'voice': 'zhixiaobai'  # 语音音色
}

SERVER_HOST = '0.0.0.0'
SERVER_PORT = 8765

# ==================== 日志配置 ====================
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# ==================== 会话管理 ====================
class SessionManager:
    """管理设备会话和百炼对话上下文"""
    
    def __init__(self):
        self.sessions: Dict[str, dict] = {}
    
    def create_session(self, device_id: str) -> str:
        """创建新会话"""
        session_id = str(uuid.uuid4())
        self.sessions[session_id] = {
            'device_id': device_id,
            'bailian_session_id': None,  # 百炼的 session_id
            'created_at': asyncio.get_event_loop().time()
        }
        logger.info(f"Created session {session_id} for device {device_id}")
        return session_id
    
    def get_session(self, session_id: str) -> Optional[dict]:
        """获取会话信息"""
        return self.sessions.get(session_id)
    
    def update_bailian_session(self, session_id: str, bailian_session_id: str):
        """更新百炼 session_id"""
        if session_id in self.sessions:
            self.sessions[session_id]['bailian_session_id'] = bailian_session_id
    
    def remove_session(self, session_id: str):
        """删除会话"""
        if session_id in self.sessions:
            del self.sessions[session_id]
            logger.info(f"Removed session {session_id}")


session_manager = SessionManager()


# ==================== 音频处理 ====================
class AudioProcessor:
    """处理 Opus 音频编解码"""
    
    @staticmethod
    async def opus_to_pcm(opus_data: bytes) -> bytes:
        """
        Opus 解码为 PCM
        需要安装: pip install opuslib
        """
        try:
            import opuslib
            
            # 创建 Opus 解码器 (16kHz, 单声道)
            decoder = opuslib.Decoder(16000, 1)
            
            # 解码 (假设 60ms 帧)
            pcm_data = decoder.decode(opus_data, frame_size=960)
            return pcm_data
        except ImportError:
            logger.error("opuslib not installed. Run: pip install opuslib")
            return b''
        except Exception as e:
            logger.error(f"Opus decode error: {e}")
            return b''
    
    @staticmethod
    async def pcm_to_opus(pcm_data: bytes, sample_rate: int = 24000) -> bytes:
        """
        PCM 编码为 Opus
        """
        try:
            import opuslib
            
            # 创建 Opus 编码器
            encoder = opuslib.Encoder(sample_rate, 1, opuslib.APPLICATION_AUDIO)
            
            # 编码 (60ms 帧)
            frame_size = sample_rate * 60 // 1000
            opus_data = encoder.encode(pcm_data, frame_size)
            return opus_data
        except Exception as e:
            logger.error(f"Opus encode error: {e}")
            return b''


# ==================== ASR 语音识别 ====================
class ASRService:
    """阿里云实时语音识别服务"""
    
    def __init__(self):
        self.audio_buffer = bytearray()
    
    async def recognize(self, opus_data: bytes) -> Optional[str]:
        """
        识别语音
        
        简化版本：这里需要对接阿里云实时语音识别 API
        参考文档: https://help.aliyun.com/document_detail/90727.html
        """
        # 1. 解码 Opus 为 PCM
        pcm_data = await AudioProcessor.opus_to_pcm(opus_data)
        if not pcm_data:
            return None
        
        # 2. 累积音频数据
        self.audio_buffer.extend(pcm_data)
        
        # 3. 当累积足够数据时，调用 ASR
        # TODO: 实际实现需要对接阿里云实时语音识别 WebSocket API
        # 这里简化为模拟识别结果
        if len(self.audio_buffer) > 16000 * 2:  # 1秒音频
            logger.info(f"ASR processing {len(self.audio_buffer)} bytes")
            self.audio_buffer.clear()
            return "你好小智"  # 模拟识别结果
        
        return None
    
    async def finalize(self) -> Optional[str]:
        """结束识别，返回最终结果"""
        if len(self.audio_buffer) > 0:
            logger.info("ASR finalizing...")
            text = "这是模拟的识别结果"  # 实际需要调用 ASR API
            self.audio_buffer.clear()
            return text
        return None


# ==================== TTS 语音合成 ====================
class TTSService:
    """阿里云语音合成服务"""
    
    @staticmethod
    async def synthesize(text: str) -> bytes:
        """
        合成语音
        
        参考文档: https://help.aliyun.com/document_detail/84435.html
        """
        try:
            # TODO: 实际实现需要对接阿里云语音合成 API
            # 这里简化为返回空数据
            logger.info(f"TTS synthesizing: {text}")
            
            # 模拟返回 Opus 音频数据
            # 实际需要：
            # 1. 调用阿里云 TTS API 获取 PCM/WAV
            # 2. 转换为 Opus 格式
            # 3. 返回 Opus 数据
            
            return b''  # 返回空数据（需要实际实现）
        except Exception as e:
            logger.error(f"TTS error: {e}")
            return b''


# ==================== 百炼应用对接 ====================
class BaiLianService:
    """阿里云百炼应用服务"""
    
    def __init__(self):
        dashscope.api_key = BAILIANCONFIG['api_key']
    
    async def chat(self, prompt: str, session_id: Optional[str] = None) -> tuple[str, str]:
        """
        调用百炼应用对话
        
        返回: (回复文本, session_id)
        """
        try:
            # 构建请求参数
            params = {
                'app_id': BAILIANCONFIG['app_id'],
                'prompt': prompt
            }
            
            # 如果有 session_id，则继续多轮对话
            if session_id:
                params['session_id'] = session_id
            
            # 调用百炼应用
            response = Application.call(**params)
            
            # 检查响应
            if response.status_code != HTTPStatus.OK:
                logger.error(f"BaiLian error: {response.status_code} - {response.message}")
                return "抱歉，我遇到了一些问题", session_id
            
            # 提取回复和 session_id
            reply_text = response.output.text
            new_session_id = response.output.session_id
            
            logger.info(f"BaiLian reply: {reply_text[:50]}... (session: {new_session_id})")
            return reply_text, new_session_id
            
        except Exception as e:
            logger.error(f"BaiLian exception: {e}")
            return "抱歉，服务出错了", session_id


# ==================== WebSocket 服务器 ====================
class XiaoZhiWebSocketServer:
    """小智设备 WebSocket 服务器"""
    
    def __init__(self):
        self.bailian = BaiLianService()
        self.asr = ASRService()
        self.tts = TTSService()
    
    async def handle_client(self, websocket, path):
        """处理设备连接"""
        device_id = None
        session_id = None
        
        try:
            logger.info(f"New client connected from {websocket.remote_address}")
            
            # 1. 接收设备 hello 消息
            hello_msg = await websocket.recv()
            hello_data = json.loads(hello_msg)
            
            logger.info(f"Received hello: {hello_data}")
            
            # 提取设备信息
            device_id = hello_data.get('device_id', 'unknown')
            
            # 创建会话
            session_id = session_manager.create_session(device_id)
            
            # 2. 回复 hello
            hello_response = {
                'type': 'hello',
                'transport': 'websocket',
                'session_id': session_id,
                'audio_params': {
                    'format': 'opus',
                    'sample_rate': 24000,
                    'channels': 1,
                    'frame_duration': 60
                }
            }
            await websocket.send(json.dumps(hello_response))
            logger.info(f"Sent hello response with session {session_id}")
            
            # 3. 处理后续消息
            async for message in websocket:
                await self.handle_message(websocket, message, session_id)
        
        except websockets.exceptions.ConnectionClosed:
            logger.info(f"Client {device_id} disconnected")
        except Exception as e:
            logger.error(f"Error handling client: {e}", exc_info=True)
        finally:
            if session_id:
                session_manager.remove_session(session_id)
    
    async def handle_message(self, websocket, message, session_id: str):
        """处理设备消息"""
        session = session_manager.get_session(session_id)
        if not session:
            logger.error(f"Session {session_id} not found")
            return
        
        # 判断消息类型
        if isinstance(message, bytes):
            # 二进制消息 = 音频数据
            await self.handle_audio(websocket, message, session_id)
        else:
            # 文本消息 = JSON 控制消息
            await self.handle_json(websocket, message, session_id)
    
    async def handle_audio(self, websocket, audio_data: bytes, session_id: str):
        """处理音频数据"""
        session = session_manager.get_session(session_id)
        
        # ASR 识别
        text = await self.asr.recognize(audio_data)
        
        if text:
            logger.info(f"ASR result: {text}")
            
            # 发送识别结果给设备
            await websocket.send(json.dumps({
                'type': 'stt',
                'text': text
            }))
            
            # 调用百炼应用
            bailian_session_id = session.get('bailian_session_id')
            reply, new_bailian_session = await self.bailian.chat(text, bailian_session_id)
            
            # 更新百炼 session
            session_manager.update_bailian_session(session_id, new_bailian_session)
            
            # 发送 TTS 开始
            await websocket.send(json.dumps({
                'type': 'tts',
                'state': 'start'
            }))
            
            # 发送回复文本（用于显示）
            await websocket.send(json.dumps({
                'type': 'tts',
                'state': 'sentence_start',
                'text': reply
            }))
            
            # TTS 合成并发送音频
            audio = await self.tts.synthesize(reply)
            if audio:
                await websocket.send(audio)  # 发送二进制音频
            
            # 发送 TTS 结束
            await websocket.send(json.dumps({
                'type': 'tts',
                'state': 'stop'
            }))
    
    async def handle_json(self, websocket, json_msg: str, session_id: str):
        """处理 JSON 控制消息"""
        try:
            data = json.loads(json_msg)
            msg_type = data.get('type')
            
            if msg_type == 'stop_listening':
                # 结束识别
                text = await self.asr.finalize()
                if text:
                    await self.handle_audio(websocket, b'', session_id)
            
            logger.info(f"Received JSON: {data}")
        except Exception as e:
            logger.error(f"Error handling JSON: {e}")
    
    async def start(self):
        """启动服务器"""
        logger.info(f"Starting WebSocket server on {SERVER_HOST}:{SERVER_PORT}")
        async with websockets.serve(self.handle_client, SERVER_HOST, SERVER_PORT):
            await asyncio.Future()  # 永久运行


# ==================== 主程序 ====================
async def main():
    """主函数"""
    server = XiaoZhiWebSocketServer()
    await server.start()


if __name__ == '__main__':
    print("""
    ╔═══════════════════════════════════════════════════════╗
    ║   小智 ESP32 + 阿里云百炼应用对接服务器               ║
    ║   WebSocket Server: ws://{}:{}              ║
    ╚═══════════════════════════════════════════════════════╝
    """.format(SERVER_HOST, SERVER_PORT))
    
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Server stopped by user")

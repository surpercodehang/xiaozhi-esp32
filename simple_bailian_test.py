#!/usr/bin/env python3
"""
简化版百炼测试服务器
用于快速验证百炼应用对接，暂不处理音频

使用方法：
1. 修改下方的 API_KEY 和 APP_ID
2. 运行: python simple_bailian_test.py
3. 配置设备连接到此服务器
4. 设备发送文本消息，服务器返回百炼回复
"""

import asyncio
import json
import logging
from http import HTTPStatus
import websockets
from dashscope import Application

# ==================== 配置 ====================
API_KEY = 'sk-3c373c69431947e781a68fd9dad86ccd'  # 替换为您的 API Key
APP_ID = 'c897a3567b374b839bb0b1974aebc548'     # 替换为您的 App ID
SERVER_PORT = 8765

# ==================== 日志 ====================
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# ==================== 会话存储 ====================
sessions = {}  # {session_id: bailian_session_id}


async def call_bailian(prompt: str, session_id: str = None):
    """调用百炼应用"""
    try:
        params = {
            'api_key': API_KEY,
            'app_id': APP_ID,
            'prompt': prompt
        }
        
        if session_id:
            params['session_id'] = session_id
        
        logger.info(f"📤 发送给百炼: {prompt}")
        response = Application.call(**params)
        
        if response.status_code != HTTPStatus.OK:
            logger.error(f"❌ 百炼错误: {response.status_code} - {response.message}")
            return "抱歉，服务出错了", session_id
        
        reply = response.output.text
        new_session = response.output.session_id
        
        logger.info(f"📥 百炼回复: {reply}")
        logger.info(f"🔑 Session ID: {new_session}")
        
        return reply, new_session
    
    except Exception as e:
        logger.error(f"❌ 异常: {e}")
        return "抱歉，发生异常", session_id


async def handle_client(websocket, path):
    """处理客户端连接"""
    client_session_id = None
    
    try:
        logger.info(f"✅ 新客户端连接: {websocket.remote_address}")
        
        # 1. 接收 hello
        hello_msg = await websocket.recv()
        logger.info(f"📨 收到消息: {hello_msg[:200]}")
        
        try:
            hello_data = json.loads(hello_msg)
            logger.info(f"📋 Hello 数据: {hello_data}")
        except:
            logger.warning("⚠️  不是 JSON 格式，跳过")
            hello_data = {}
        
        # 生成会话 ID
        import uuid
        client_session_id = str(uuid.uuid4())
        sessions[client_session_id] = None
        
        # 2. 回复 hello
        hello_response = {
            'type': 'hello',
            'transport': 'websocket',
            'session_id': client_session_id,
            'audio_params': {
                'format': 'opus',
                'sample_rate': 24000,
                'channels': 1,
                'frame_duration': 60
            }
        }
        
        await websocket.send(json.dumps(hello_response))
        logger.info(f"📤 发送 hello 响应")
        
        # 3. 处理消息循环
        async for message in websocket:
            if isinstance(message, bytes):
                # 二进制消息（音频）- 暂时忽略
                logger.info(f"🎵 收到音频数据: {len(message)} 字节 (暂不处理)")
                
                # 模拟识别结果
                test_text = "你好"
                logger.info(f"🎤 模拟识别: {test_text}")
                
                # 发送识别结果
                await websocket.send(json.dumps({
                    'type': 'stt',
                    'text': test_text
                }))
                
                # 调用百炼
                bailian_session = sessions.get(client_session_id)
                reply, new_session = await call_bailian(test_text, bailian_session)
                sessions[client_session_id] = new_session
                
                # 发送回复
                await websocket.send(json.dumps({
                    'type': 'tts',
                    'state': 'start'
                }))
                
                await websocket.send(json.dumps({
                    'type': 'tts',
                    'state': 'sentence_start',
                    'text': reply
                }))
                
                # 模拟音频（空数据）
                # await websocket.send(b'')
                
                await websocket.send(json.dumps({
                    'type': 'tts',
                    'state': 'stop'
                }))
                
            else:
                # 文本消息（JSON）
                try:
                    data = json.loads(message)
                    logger.info(f"📨 收到 JSON: {data}")
                    
                    msg_type = data.get('type')
                    
                    if msg_type == 'text':
                        # 处理文本消息
                        text = data.get('text', '')
                        
                        # 调用百炼
                        bailian_session = sessions.get(client_session_id)
                        reply, new_session = await call_bailian(text, bailian_session)
                        sessions[client_session_id] = new_session
                        
                        # 发送回复
                        await websocket.send(json.dumps({
                            'type': 'text',
                            'text': reply
                        }))
                
                except json.JSONDecodeError:
                    logger.warning(f"⚠️  无效 JSON: {message[:100]}")
    
    except websockets.exceptions.ConnectionClosed:
        logger.info(f"❌ 客户端断开连接")
    
    except Exception as e:
        logger.error(f"❌ 错误: {e}", exc_info=True)
    
    finally:
        if client_session_id and client_session_id in sessions:
            del sessions[client_session_id]
            logger.info(f"🗑️  清理会话: {client_session_id}")


async def main():
    """启动服务器"""
    logger.info(f"""
╔═══════════════════════════════════════════════════════╗
║     小智 ESP32 + 百炼应用 简化测试服务器              ║
║                                                       ║
║     WebSocket: ws://0.0.0.0:{SERVER_PORT}                    ║
║     百炼 App ID: {APP_ID[:20]}...        ║
║                                                       ║
║     功能：仅测试百炼对话，暂不处理音频                 ║
╚═══════════════════════════════════════════════════════╝
    """)
    
    async with websockets.serve(handle_client, '0.0.0.0', SERVER_PORT):
        logger.info(f"🚀 服务器启动成功！")
        await asyncio.Future()  # 永久运行


if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("\n👋 服务器已停止")

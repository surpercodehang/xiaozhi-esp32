#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
阿里云百炼 API 配置工具

通过串口配置 ESP32 设备的百炼 API 参数

使用方法:
    python configure_bailian.py <port> <api_key> <app_id>

示例:
    python configure_bailian.py COM3 sk-xxx your-app-id
    python configure_bailian.py /dev/ttyUSB0 sk-xxx your-app-id
"""

import sys
import serial
import json
import time

def configure_bailian(port, api_key, app_id, endpoint=None):
    """
    通过串口配置百炼 API
    
    Args:
        port: 串口号 (e.g., COM3, /dev/ttyUSB0)
        api_key: API Key
        app_id: 应用 ID
        endpoint: API 端点 (可选)
    """
    try:
        # 打开串口
        print(f"Opening serial port: {port}")
        ser = serial.Serial(port, 115200, timeout=2)
        time.sleep(0.5)  # 等待串口稳定
        
        # 构造配置命令
        config = {
            "cmd": "set_bailian_config",
            "api_key": api_key,
            "app_id": app_id
        }
        
        if endpoint:
            config["endpoint"] = endpoint
        
        # 发送配置命令
        command = json.dumps(config) + '\n'
        print(f"Sending configuration...")
        print(f"  API Key: {api_key[:10]}...")
        print(f"  App ID: {app_id}")
        if endpoint:
            print(f"  Endpoint: {endpoint}")
        
        ser.write(command.encode())
        
        # 等待响应
        print("Waiting for response...")
        response = ser.readline().decode().strip()
        
        if response:
            print(f"Response: {response}")
            try:
                resp_json = json.loads(response)
                if resp_json.get("status") == "ok":
                    print("\n✓ Configuration saved successfully!")
                    return True
                else:
                    print(f"\n✗ Configuration failed: {resp_json.get('message')}")
                    return False
            except json.JSONDecodeError:
                print(f"\n✗ Invalid response format: {response}")
                return False
        else:
            print("\n✗ No response from device (timeout)")
            return False
        
    except serial.SerialException as e:
        print(f"\n✗ Serial port error: {e}")
        return False
    except Exception as e:
        print(f"\n✗ Error: {e}")
        return False
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial port closed")

def main():
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(1)
    
    port = sys.argv[1]
    api_key = sys.argv[2]
    app_id = sys.argv[3]
    endpoint = sys.argv[4] if len(sys.argv) > 4 else None
    
    success = configure_bailian(port, api_key, app_id, endpoint)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()

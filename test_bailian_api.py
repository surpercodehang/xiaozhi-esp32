#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试阿里云百炼 API
用于验证 API Key 和 App ID 是否正确
"""

import requests
import json

# 你的凭证
API_KEY = "sk-3c373c69431947e781a68fd9dad86ccd"
APP_ID = "c897a3567b374b839bb0b1974aebc548"
ENDPOINT = "https://dashscope.aliyuncs.com/api/v1/apps"

def test_bailian_api():
    url = f"{ENDPOINT}/{APP_ID}/completion"
    
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json"
    }
    
    data = {
        "prompt": "你好,今天天气怎么样?"
    }
    
    print(f"测试 URL: {url}")
    print(f"API Key (前10位): {API_KEY[:10]}...")
    print(f"请求数据: {json.dumps(data, ensure_ascii=False)}")
    print("\n发送请求...")
    
    try:
        response = requests.post(url, headers=headers, json=data)
        
        print(f"\n状态码: {response.status_code}")
        print(f"响应头: {dict(response.headers)}")
        print(f"\n响应内容:")
        print(json.dumps(response.json(), ensure_ascii=False, indent=2))
        
        if response.status_code == 200:
            print("\n✅ 测试成功! API Key 和 App ID 都是有效的")
        elif response.status_code == 401:
            print("\n❌ 401 错误: API Key 无效或过期")
            print("请检查:")
            print("1. API Key 是否正确")
            print("2. API Key 是否已激活")
            print("3. API Key 是否有权限访问该应用")
        elif response.status_code == 404:
            print("\n❌ 404 错误: App ID 无效或不存在")
        else:
            print(f"\n❌ 错误: HTTP {response.status_code}")
            
    except Exception as e:
        print(f"\n❌ 请求失败: {e}")

if __name__ == "__main__":
    test_bailian_api()

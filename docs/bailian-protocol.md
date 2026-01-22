# 阿里云百炼应用协议使用指南

## 简介

小智 AI 聊天机器人现在支持直接使用阿里云百炼应用的 API Key + App ID,无需通过 xiaozhi.me 官网配置智能体。

这种方式具有以下优势:

- ✅ **更简单**: 不需要在官网配置,直接使用 API
- ✅ **更灵活**: 可以自定义百炼应用的所有参数
- ✅ **更直接**: 减少中间环节,降低延迟
- ✅ **多轮对话**: 自动支持 session_id 实现多轮对话
- ✅ **流式输出**: 支持流式音频输出,降低首字节延迟

## 前置准备

### 1. 获取阿里云百炼 API Key

1. 访问 [阿里云百炼控制台](https://dashscope.console.aliyun.com/)
2. 注册/登录阿里云账号
3. 创建 API Key (会得到一个以 `sk-` 开头的密钥)
4. 保存好这个 API Key

### 2. 创建百炼应用

1. 访问 [百炼应用控制台](https://bailian.console.aliyun.com/)
2. 点击"创建应用"
3. 配置应用:
   - **输入类型**: 选择"音频"
   - **输出类型**: 选择"音频"
   - **模型**: 选择支持实时语音的模型 (如 qwen-omni-turbo)
   - **语音合成**: 配置 TTS 参数
4. 创建后会得到一个 App ID

### 3. 准备硬件

确保你有以下硬件:
- ESP32-S3 开发板(推荐,也支持 ESP32-C3/P4)
- 麦克风 (如 INMP441)
- 扬声器/功放 (如 MAX98357A)

## 配置步骤

### 方法一: 使用 menuconfig (推荐)

1. 进入项目目录:
```bash
cd xiaozhi-esp32
```

2. 运行配置工具:
```bash
idf.py menuconfig
```

3. 进入 `Xiaozhi Assistant` 菜单

4. 选择 `Protocol Type` → 选择 `Use Aliyun Bailian Application (Direct API)`

5. 进入 `Aliyun Bailian Application Configuration` 子菜单

6. 配置以下选项:
   - `Bailian API Key`: 填入你的 API Key (以 sk- 开头)
   - `Bailian Application ID`: 填入你的 App ID
   - `Enable Streaming Output`: 建议开启
   - `Session Timeout`: 默认 300 秒即可

7. 按 `S` 保存,按 `Q` 退出

8. 编译并烧录:
```bash
idf.py build flash monitor
```

### 方法二: 使用示例配置文件

1. 复制示例配置文件:
```bash
cp sdkconfig.bailian.example sdkconfig.defaults
```

2. 编辑 `sdkconfig.defaults`,修改以下行:
```
CONFIG_BAILIAN_API_KEY="sk-你的实际API_Key"
CONFIG_BAILIAN_APP_ID="你的实际App_ID"
```

3. 选择你的开发板类型(取消注释对应的行):
```
# 例如使用立创实战派 ESP32-S3
CONFIG_BOARD_TYPE_LICHUANG_DEV_S3=y
```

4. 编译并烧录:
```bash
idf.py build flash monitor
```

### 方法三: 通过 NVS 动态配置

如果不想在编译时写死配置,可以通过 NVS 动态配置:

```c
#include "nvs_flash.h"
#include "nvs.h"

void configure_bailian() {
    nvs_handle_t handle;
    nvs_open("bailian", NVS_READWRITE, &handle);
    
    nvs_set_str(handle, "api_key", "sk-your-api-key");
    nvs_set_str(handle, "app_id", "your-app-id");
    
    nvs_commit(handle);
    nvs_close(handle);
}
```

## 使用流程

### 1. 设备启动

设备启动后会:
1. 连接 WiFi
2. 初始化百炼协议(不会连接 xiaozhi.me)
3. 进入待机状态

### 2. 语音交互

1. 说出唤醒词 "你好小智"
2. 听到提示音后开始说话
3. 设备将音频实时发送到百炼应用
4. 百炼应用流式返回语音回复
5. 设备播放 AI 回复

### 3. 多轮对话

首次对话后,系统会保存 session_id,支持连续多轮对话:

```
用户: "你好小智" (唤醒)
AI: "你好,我在!"
用户: "今天天气怎么样?"
AI: "今天天气晴朗..." (保存 session_id)
用户: "明天呢?" (使用同一 session_id)
AI: "明天可能会下雨..." (理解上下文)
```

## 与官方服务器方式的对比

| 特性 | 百炼应用协议 | xiaozhi.me 官方服务器 |
|------|------------|---------------------|
| 配置方式 | API Key + App ID | 官网配置智能体 |
| OTA 升级 | ❌ 不支持 | ✅ 支持 |
| 多轮对话 | ✅ 自动支持 | ✅ 支持 |
| 流式输出 | ✅ 支持 | ✅ 支持 |
| 配置界面 | 代码配置 | 网页配置 |
| MCP 协议 | ❌ 暂不支持 | ✅ 支持 |
| 自定义提示词 | 百炼应用配置 | 官网配置 |
| 成本 | 阿里云计费 | 官方额度 |

## 故障排除

### 问题 1: 连接失败

**现象**: 设备无法连接到百炼服务

**解决方案**:
1. 检查 API Key 是否正确 (以 `sk-` 开头)
2. 检查 App ID 是否正确
3. 检查网络连接是否正常
4. 检查阿里云账户是否有额度
5. 查看串口日志获取详细错误信息:
```bash
idf.py monitor
```

### 问题 2: 无法识别语音

**现象**: 说话后没有反应

**解决方案**:
1. 检查麦克风接线是否正确
2. 检查百炼应用是否配置为支持音频输入
3. 检查音频格式是否匹配 (16kHz, 单声道, PCM)
4. 增加麦克风音量或靠近麦克风说话

### 问题 3: 播放音频失败

**现象**: AI 有回复但没有声音

**解决方案**:
1. 检查扬声器/功放接线是否正确
2. 检查百炼应用是否配置为输出音频
3. 检查音频解码器是否正常初始化
4. 查看串口日志中的音频相关错误

### 问题 4: 多轮对话失败

**现象**: AI 无法理解上下文

**解决方案**:
1. 检查 session_id 是否正确保存
2. 检查会话超时设置 (默认 300 秒)
3. 在百炼应用中开启多轮对话功能

### 问题 5: 延迟过高

**现象**: 说完话后等很久才有回复

**解决方案**:
1. 确保启用了流式输出 (`CONFIG_BAILIAN_USE_STREAMING=y`)
2. 检查网络延迟 (ping dashscope.aliyuncs.com)
3. 使用更快的模型 (如 qwen-omni-turbo)
4. 检查是否开启了实时传输

## 高级配置

### 自定义百炼协议类

如果需要更高级的定制,可以修改 `main/protocols/bailian_protocol.cc`:

```cpp
// 修改音频参数
int server_sample_rate_ = 24000;  // 改为 16000
int server_frame_duration_ = 20;  // 改为 10

// 修改超时时间
bool IsTimeout() const {
    // ... 自定义超时逻辑
}

// 添加自定义事件处理
void ParseServerResponse(cJSON* root) {
    // ... 自定义解析逻辑
}
```

### 集成其他服务

百炼协议类继承自 `Protocol` 基类,你可以参考它实现其他服务的协议:

```cpp
class MyCustomProtocol : public Protocol {
public:
    bool OpenAudioChannel() override;
    bool SendAudio(std::unique_ptr<AudioStreamPacket> packet) override;
    // ... 实现其他接口
};
```

## 示例代码

### Python 服务器示例

参考 `speech_commands_recognition_with_llm/server/server.py`,你也可以自己搭建服务器:

```python
from http import HTTPStatus
from dashscope import Application

def call_bailian_app():
    response = Application.call(
        api_key='sk-your-api-key',
        app_id='your-app-id',
        prompt='你是谁?',
        stream=True,
        incremental_output=True
    )
    
    for chunk in response:
        if chunk.status_code == HTTPStatus.OK:
            print(chunk.output.text)
```

### ESP32 客户端示例

已经集成在主项目中,参考 `main/protocols/bailian_protocol.cc`

## 贡献

如果你发现 bug 或有改进建议,欢迎提交 Issue 或 Pull Request!

## 相关链接

- [阿里云百炼控制台](https://bailian.console.aliyun.com/)
- [DashScope API 文档](https://help.aliyun.com/zh/dashscope/)
- [小智 ESP32 项目主页](https://github.com/78/xiaozhi-esp32)
- [语音识别示例项目](../speech_commands_recognition_with_llm/README.md)

## 许可证

MIT License - 与主项目保持一致

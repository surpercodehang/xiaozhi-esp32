# 阿里云百炼协议集成指南

## 概述

小智AI聊天机器人现在支持通过阿里云百炼（DashScope）协议直接配置大模型，无需通过官网配置服务器。

## 配置步骤

### 1. 获取阿里云百炼凭证

1. 访问 [阿里云百炼控制台](https://bailian.console.aliyun.com/)
2. 创建或选择一个智能体应用
3. 复制应用的 App ID
4. 在[密钥管理页面](https://bailian.console.aliyun.com/?apiKey=1#/api-key)创建并获取 API Key

### 2. 配置固件

有两种配置方式：

#### 方式一：在Kconfig中静态配置（推荐用于生产）

在 `menuconfig` 中选择：
```
Xiaozhi Assistant > Protocol Type > Aliyun DashScope Protocol
```

然后配置以下参数：
- **DashScope API Key**: 输入您的API Key
- **DashScope App ID**: 输入应用ID
- **DashScope Base URL**: 默认 `https://dashscope.aliyuncs.com/api/v1/`

#### 方式二：通过Settings动态配置

如果需要在运行时配置，可以通过Settings API设置：

```cpp
Settings settings("dashscope", true);
settings.SetString("api_key", "your-api-key");
settings.SetString("app_id", "your-app-id");
settings.SetString("base_url", "https://dashscope.aliyuncs.com/api/v1/");
```

### 3. 编译和烧录

```bash
idf.py menuconfig  # 配置协议和参数
idf.py build
idf.py flash
```

## 功能特性

- ✅ 支持流式对话
- ✅ 自动会话管理
- ✅ 多轮对话上下文保持
- ✅ 错误处理和重试机制
- ✅ 与现有语音交互完美集成

## 协议工作流程

1. **初始化**: 检查API Key和App ID配置
2. **音频通道**: 建立HTTP连接准备发送请求
3. **语音输入**: 将语音转换为文本发送给DashScope API
4. **流式响应**: 实时接收并播放AI回复
5. **上下文管理**: 维护对话历史，支持多轮对话

## 故障排除

### 常见问题

1. **连接失败**
   - 检查网络连接
   - 验证API Key和App ID是否正确
   - 确认Base URL格式正确

2. **认证失败**
   - 检查API Key是否有效且有足够权限
   - 确认App ID对应的应用存在且已发布

3. **响应异常**
   - 查看设备日志中的错误信息
   - 检查阿里云百炼控制台的应用配置

### 日志调试

启用详细日志查看通信过程：
```cpp
esp_log_level_set("DashScope", ESP_LOG_DEBUG);
```

## API参考

### 支持的配置参数

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| api_key | string | 是 | 阿里云百炼API Key |
| app_id | string | 是 | 智能体应用ID |
| base_url | string | 否 | API基础URL，默认为DashScope官方地址 |

### 响应格式

AI回复通过标准的JSON格式传递给应用层：

```json
{
  "type": "speech",
  "text": "AI回复内容",
  "session_id": "会话ID"
}
```

## 安全注意事项

- 🔒 API Key请妥善保管，不要在代码中硬编码
- 🔒 建议在生产环境中使用环境变量或安全存储
- 🔒 定期轮换API Key以确保安全性
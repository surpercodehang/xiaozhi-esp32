# 快速开始: 使用阿里云百炼应用

## 5 分钟快速上手

### 第 1 步: 获取 API 密钥

1. 打开 [阿里云百炼控制台](https://dashscope.console.aliyun.com/)
2. 点击"创建 API-KEY"
3. 复制生成的 API Key (形如 `sk-xxxxxxxx`)

### 第 2 步: 创建应用

1. 打开 [百炼应用控制台](https://bailian.console.aliyun.com/)
2. 点击"创建应用"
3. 配置应用:
   - 名称: `小智语音助手`
   - 输入: 选择"音频"
   - 输出: 选择"音频" + "流式输出"
   - 模型: `qwen-omni-turbo-realtime-2025-05-08`
4. 复制应用 ID

### 第 3 步: 配置项目

复制示例配置并修改:

```bash
cp sdkconfig.bailian.example sdkconfig.defaults
```

编辑 `sdkconfig.defaults`,填入你的信息:

```ini
# 替换为你的 API Key
CONFIG_BAILIAN_API_KEY="sk-你的API密钥"

# 替换为你的 App ID  
CONFIG_BAILIAN_APP_ID="你的应用ID"

# 选择你的开发板(取消注释对应行)
CONFIG_BOARD_TYPE_LICHUANG_DEV_S3=y
```

### 第 4 步: 编译烧录

```bash
idf.py build flash monitor
```

### 第 5 步: 开始使用

1. 设备启动后连接 WiFi
2. 说 "你好小智" 唤醒设备
3. 听到提示音后说话
4. AI 会实时回复你!

## 与传统方式的区别

### 传统方式 (xiaozhi.me)
```
ESP32 → xiaozhi.me服务器 → 大模型
         ↑需要官网配置智能体
```

### 百炼应用方式 (新)
```
ESP32 → 阿里云百炼 API → 大模型
         ↑直接使用API,无需官网配置
```

## 优势对比

| 项目 | 百炼应用 | 传统方式 |
|-----|---------|---------|
| 配置复杂度 | ⭐ 简单 | ⭐⭐ 需要官网配置 |
| 响应速度 | ⭐⭐⭐ 直连快 | ⭐⭐ 经过转发 |
| 自定义能力 | ⭐⭐⭐ 完全控制 | ⭐⭐ 受限于官网 |
| OTA升级 | ❌ 不支持 | ✅ 支持 |
| 成本 | 阿里云计费 | 官方免费额度 |

## 常见问题

**Q: 连接失败怎么办?**  
A: 检查 API Key 和 App ID 是否正确,确保阿里云账户有余额。

**Q: 支持多轮对话吗?**  
A: 支持!会自动保存 session_id。

**Q: 可以自定义 AI 的性格吗?**  
A: 可以!在百炼应用控制台配置系统提示词。

**Q: 能用其他大模型吗?**  
A: 可以!百炼支持多种模型,在应用配置中选择。

**Q: 延迟高怎么办?**  
A: 确保开启了流式输出,使用实时模型。

## 下一步

- 📖 阅读 [详细文档](bailian-protocol.md)
- 🔧 参考 [speech_commands_recognition_with_llm](../speech_commands_recognition_with_llm/README.md) 示例
- 💬 加入 QQ 群 1011329060 交流
- 🐛 遇到问题? [提交 Issue](https://github.com/78/xiaozhi-esp32/issues)

## 示例应用配置

在百炼应用控制台,你可以这样配置:

### 基础配置
```
名称: 小智AI助手
描述: 一个能听会说的智能助手
```

### 系统提示词
```
你是小智,一个友好、热情的AI助手。
你的回答简洁明了,不超过100字。
你擅长回答日常问题,提供实用建议。
```

### 输入配置
```
类型: 音频
格式: PCM
采样率: 16000 Hz
声道: 单声道
```

### 输出配置
```
类型: 音频
格式: PCM
采样率: 24000 Hz
流式输出: 开启
语音合成: Chelsie (或其他你喜欢的音色)
```

### 高级配置
```
温度: 0.7
最大token数: 500
多轮对话: 开启
```

## 费用说明

阿里云百炼采用按量计费:

- **语音识别**: 约 ¥0.001/次
- **大模型推理**: 约 ¥0.002/次  
- **语音合成**: 约 ¥0.001/次

**总计**: 每次对话约 ¥0.004 (不到 1 分钱)

新用户通常有免费额度,足够测试使用!

## 技术支持

- 📧 邮件: [创建 Issue](https://github.com/78/xiaozhi-esp32/issues)
- 💬 QQ群: 1011329060
- 📺 B站: [@虾哥说AI](https://space.bilibili.com/example)

---

**开始你的 AI 语音助手之旅吧!** 🚀

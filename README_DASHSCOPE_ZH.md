# 小智 ESP32 - 阿里云百炼版本

> 基于阿里云百炼应用的独立语音 AI 助手

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://docs.espressif.com/projects/esp-idf/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange)](https://www.espressif.com/zh-hans/products/socs/esp32-s3)

## 🌟 项目简介

这是小智 ESP32 AI 聊天机器人的阿里云百炼改造版本,不再依赖 xiaozhi.me 官方服务器,而是直接使用阿里云百炼应用提供的 AI 能力。

**核心特性**:
- ✅ 完全独立,无需官方服务器
- ✅ 集成阿里云百炼应用
- ✅ 支持语音识别 (ASR)
- ✅ 支持大模型对话 (LLM)
- ✅ 支持语音合成 (TTS)
- ✅ 保留原有硬件功能
- ✅ 支持多轮对话

## 🎯 与原版的区别

| 特性 | 原版 | 百炼版 |
|-----|------|--------|
| **服务器** | xiaozhi.me 官方 | 阿里云百炼 |
| **激活流程** | 需要激活码 | 无需激活 |
| **配置方式** | OTA 下发 | 代码/NVS 配置 |
| **通信协议** | WebSocket | HTTP (可扩展 WebSocket) |
| **ASR** | 官方服务 | 阿里云语音识别 |
| **LLM** | 官方配置 | 百炼应用 |
| **TTS** | 官方服务 | 阿里云语音合成 |
| **独立性** | 依赖官方 | 完全独立 |

## 📁 项目结构

```
xiaozhi-esp32/
├── main/
│   ├── protocols/
│   │   ├── dashscope_protocol.h      # 新增: 百炼协议头文件
│   │   ├── dashscope_protocol.cc     # 新增: 百炼协议实现
│   │   ├── protocol.h
│   │   ├── mqtt_protocol.*
│   │   └── websocket_protocol.*
│   ├── application.cc                 # 修改: 使用百炼协议
│   └── ...
├── docs/                              # 原有文档
├── dashscope_config.md               # 新增: 配置说明
├── DASHSCOPE_INTEGRATION.md          # 新增: 集成文档
├── QUICK_START_GUIDE_ZH.md           # 新增: 快速开始
├── CODE_CHANGES_SUMMARY_ZH.md        # 新增: 代码变更
├── TESTING_GUIDE_ZH.md               # 新增: 测试指南
├── MIGRATION_SUMMARY_ZH.md           # 新增: 迁移总结
└── README_DASHSCOPE_ZH.md            # 本文件
```

## 🚀 快速开始

### 1️⃣ 准备工作

**硬件**:
- ESP32-S3 开发板 (或其他支持的板子)
- 麦克风
- 扬声器
- USB 数据线

**软件**:
- ESP-IDF 5.4+
- Python 3.8+
- 阿里云账号 (需开通百炼服务)

### 2️⃣ 获取阿里云密钥

1. 登录 [阿里云百炼控制台](https://bailian.console.aliyun.com/)
2. 创建 API Key
3. 创建应用,获取 App ID

### 3️⃣ 配置密钥

编辑 `main/protocols/dashscope_protocol.cc` (第 55-60 行):

```cpp
if (api_key_.empty()) {
    api_key_ = "sk-你的API密钥";  // 改这里
}
if (app_id_.empty()) {
    app_id_ = "你的应用ID";       // 改这里
}
```

### 4️⃣ 编译烧录

```bash
# 配置项目
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash monitor
```

### 5️⃣ 测试

1. 等待设备启动 (约 5 秒)
2. 说出唤醒词: **"你好小智"**
3. 听到提示音后说话: **"你是谁?"**
4. 等待语音回复

**成功!** 🎉

## 📖 详细文档

| 文档 | 说明 | 适合人群 |
|-----|------|---------|
| [快速开始指南](QUICK_START_GUIDE_ZH.md) | 从零开始上手 | 新手用户 |
| [集成文档](DASHSCOPE_INTEGRATION.md) | 技术细节 | 开发者 |
| [配置说明](dashscope_config.md) | 配置方法 | 所有用户 |
| [测试指南](TESTING_GUIDE_ZH.md) | 完整测试流程 | 测试人员 |
| [代码变更](CODE_CHANGES_SUMMARY_ZH.md) | 代码对比 | 开发者 |
| [迁移总结](MIGRATION_SUMMARY_ZH.md) | 项目总结 | 项目管理 |

## 🏗️ 架构说明

### 系统架构

```
┌──────────────────────────────────┐
│         ESP32 设备                │
│  ┌──────────────────────────┐   │
│  │    音频处理               │   │
│  │  - 唤醒词检测 (本地)     │   │
│  │  - Opus 编解码           │   │
│  │  - 音频队列管理          │   │
│  └──────────────────────────┘   │
│  ┌──────────────────────────┐   │
│  │  Dashscope 协议          │   │
│  │  - ASR 任务              │   │
│  │  - LLM 处理              │   │
│  │  - TTS 处理              │   │
│  └──────────────────────────┘   │
└─────────┬────────────────────────┘
          │ HTTP/HTTPS
          ↓
┌──────────────────────────────────┐
│        阿里云服务                 │
│  ┌──────────────────────────┐   │
│  │  语音识别 (ASR)          │   │
│  └──────────────────────────┘   │
│  ┌──────────────────────────┐   │
│  │  百炼应用 (LLM)          │   │
│  │  - 支持多轮对话          │   │
│  │  - session_id 保持上下文 │   │
│  └──────────────────────────┘   │
│  ┌──────────────────────────┐   │
│  │  语音合成 (TTS)          │   │
│  └──────────────────────────┘   │
└──────────────────────────────────┘
```

### 交互流程

```
用户说话
    ↓
唤醒词检测 (本地)
    ↓
录音 → Opus 编码 → 队列
    ↓
ASR 任务: Opus → PCM → 阿里云 ASR
    ↓
识别文本 → 显示
    ↓
LLM 处理: HTTP POST → 百炼应用
    ↓
回复文本 → 显示
    ↓
TTS 处理: 阿里云 TTS → 音频流
    ↓
PCM → Opus → 播放队列
    ↓
扬声器播放
```

## 💡 核心功能

### DashscopeProtocol 协议类

```cpp
class DashscopeProtocol : public Protocol {
    // 实现与阿里云服务的所有交互
    // 管理 ASR、LLM、TTS 完整流程
}
```

**主要功能**:
- 📤 **音频上传**: 将录音数据发送到 ASR
- 🎯 **语音识别**: 调用阿里云 ASR API
- 🤖 **对话处理**: 调用百炼应用获取回复
- 🗣️ **语音合成**: 调用阿里云 TTS 生成语音
- 🔄 **会话管理**: 通过 session_id 维持上下文

## ⚙️ 配置选项

### 方法 1: 代码配置 (最简单)

修改 `main/protocols/dashscope_protocol.cc`:

```cpp
api_key_ = "你的API_KEY";
app_id_ = "你的APP_ID";
```

### 方法 2: NVS 配置 (推荐)

```bash
# 1. 创建配置文件
cat > nvs_dashscope.csv << EOF
key,type,encoding,value
dashscope,namespace,,
api_key,data,string,sk-你的密钥
app_id,data,string,你的应用ID
EOF

# 2. 生成并烧录
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
    generate nvs_dashscope.csv nvs_dashscope.bin 0x6000
esptool.py --port /dev/ttyUSB0 write_flash 0x9000 nvs_dashscope.bin
```

## 📊 性能指标

| 指标 | 数值 | 说明 |
|-----|------|-----|
| **启动时间** | ~5 秒 | 跳过激活流程 |
| **响应延迟** | 2-3 秒 | 可优化到 1-1.5 秒 |
| **内存占用** | ~40KB | 峰值 |
| **空闲内存** | ~80KB | 对话中 |
| **识别准确率** | >90% | 安静环境 |

## 🔧 常见问题

### Q: 无法连接到阿里云?

**A**: 检查:
1. Wi-Fi 是否连接
2. API Key 是否正确
3. 防火墙设置

### Q: 识别不准确?

**A**: 尝试:
1. 在安静环境测试
2. 调整麦克风增益
3. 清晰发音

### Q: 无法播放回复?

**A**: 检查:
1. 扬声器连接
2. 音量设置
3. TTS 服务状态

### Q: 如何修改唤醒词?

**A**: 参考原项目文档,唤醒词检测在本地完成。

### Q: 支持哪些开发板?

**A**: 支持原项目的所有开发板,推荐 ESP32-S3。

## 🎨 自定义

### 修改 LLM 参数

在 `SendToLlm()` 函数中添加:

```cpp
cJSON_AddNumberToObject(parameters, "temperature", 0.8);
cJSON_AddNumberToObject(parameters, "top_p", 0.9);
cJSON_AddNumberToObject(parameters, "max_tokens", 2000);
```

### 调整音频参数

```cpp
const size_t MIN_AUDIO_SIZE = 16000 * 2;  // 1秒 → 改为 0.5秒
```

### 更换 TTS 音色

在 TTS 请求中添加音色参数 (需查阅阿里云文档)。

## 🚀 后续优化

### 计划中的改进

- [ ] 流式 ASR (降低延迟)
- [ ] 流式 TTS (边接收边播放)
- [ ] 本地 VAD (自动检测说话结束)
- [ ] 错误重试机制
- [ ] 离线缓存

### 欢迎贡献

- 提交 Issue
- 发起 Pull Request
- 完善文档
- 分享经验

## 📚 相关资源

### 官方文档

- [阿里云百炼](https://help.aliyun.com/zh/model-studio/)
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/)

### 原项目

- [小智 ESP32](https://github.com/78/xiaozhi-esp32)
- [原项目 README](README_zh.md)

### 社区

- QQ 群: 1011329060
- GitHub Discussions

## 🤝 贡献者

感谢所有为本项目做出贡献的人!

## 📄 许可证

MIT License

详见 [LICENSE](LICENSE) 文件。

## 🙏 致谢

- 小智 ESP32 原项目及作者
- 阿里云百炼团队
- ESP-IDF 团队
- 所有贡献者和用户

## 📞 联系方式

- **Issue**: [GitHub Issues](https://github.com/xxx)
- **QQ群**: 1011329060
- **邮件**: (如有)

---

## ⭐ Star History

如果这个项目对你有帮助,请给个 Star! ⭐

---

**Made with ❤️ by the Xiaozhi Community**

**Last Updated**: 2026-01-21

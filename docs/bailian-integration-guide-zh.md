# 小智 AI - 阿里云百炼集成指南

## 📋 目录

1. [当前架构说明](#当前架构说明)
2. [新架构方案](#新架构方案)
3. [快速开始](#快速开始)
4. [详细配置](#详细配置)
5. [常见问题](#常见问题)

---

## 当前架构说明

### 现有流程

```
📱 ESP32设备 ──WebSocket──> 🖥️ xiaozhi.me服务器 ──HTTP──> 🤖 大模型API
```

**工作方式:**
1. 设备通过 OTA URL 获取服务器地址
2. 设备连接到 xiaozhi.me 服务器
3. 服务器调用 Qwen/DeepSeek API
4. 结果返回给设备

**配置位置:**
- 配置存储在 NVS (非易失性存储)
- 命名空间: `websocket` 或 `mqtt`
- 关键字段:
  - `url` - 服务器地址
  - `token` - 认证令牌

---

## 新架构方案

### 直接调用百炼 API

```
📱 ESP32设备 ──HTTPS──> ☁️ 阿里云百炼API
```

**优势:**
- ✅ 无需自建服务器
- ✅ 配置简单,只需 API Key + App ID
- ✅ 官方服务,稳定可靠
- ✅ 支持多种高级功能

---

## 快速开始

### 步骤 1: 获取阿里云百炼凭证

1. **获取 API Key**
   - 登录 [阿里云控制台](https://dashscope.console.aliyun.com/)
   - 进入「模型服务灵积」
   - 点击「密钥管理」
   - 创建并复制 API Key (格式: `sk-xxx`)

2. **创建智能体应用**
   - 进入 [百炼控制台](https://bailian.console.aliyun.com/)
   - 点击「应用管理」
   - 创建新应用
   - 复制应用 ID (App ID)

### 步骤 2: 修改项目配置

在项目根目录下创建 `sdkconfig.defaults.bailian`:

```ini
# 选择百炼 API 模式
CONFIG_API_MODE_ALIYUN_BAILIAN=y

# 配置百炼 API
CONFIG_BAILIAN_API_KEY="sk-你的API密钥"
CONFIG_BAILIAN_APP_ID="你的应用ID"
CONFIG_BAILIAN_ENDPOINT="https://dashscope.aliyuncs.com/api/v1/apps"

# 启用流式输出
CONFIG_BAILIAN_ENABLE_STREAM=y
CONFIG_BAILIAN_ENABLE_INCREMENTAL=y

# 可选:启用思考过程(DeepSeek-R1等模型)
CONFIG_BAILIAN_ENABLE_THOUGHTS=n
```

### 步骤 3: 编译烧录

```bash
# 配置项目
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py -p COM3 flash monitor
```

---

## 详细配置

### 1. Menuconfig 配置

运行 `idf.py menuconfig` 后:

```
Xiaozhi Assistant
  → API Mode
      (X) Aliyun Bailian API        <-- 选择这个
      ( ) XiaoZhi Official Server
      ( ) Custom Server
  
  → Aliyun Bailian Configuration
      API Key: sk-xxx
      Application ID: xxx
      API Endpoint: https://dashscope.aliyuncs.com/api/v1/apps
      [*] Enable Stream Output
      [*] Enable Incremental Output  
      [ ] Enable Thoughts Output
```

### 2. NVS 运行时配置

设备运行后,可以通过 NVS 动态修改配置:

**命名空间:** `bailian`

| 键名 | 类型 | 说明 | 示例值 |
|------|------|------|--------|
| `api_key` | String | API密钥 | `sk-xxx` |
| `app_id` | String | 应用ID | `xxxxxx` |
| `endpoint` | String | API端点 | `https://...` |
| `session_id` | String | 会话ID(自动管理) | `xxx` |
| `memory_id` | String | 长期记忆ID | `xxx` |
| `stream` | Bool | 流式输出 | `true` |
| `incremental` | Bool | 增量输出 | `true` |
| `has_thoughts` | Bool | 思考过程 | `false` |

### 3. 代码示例

#### 基本调用

```cpp
#include "bailian_api_client.h"

BailianApiClient client;

// 初始化
if (!client.Initialize()) {
    ESP_LOGE(TAG, "初始化失败");
    return;
}

// 发送请求
client.SendPrompt(
    "你好,请介绍一下你自己",
    [](const BailianApiClient::Response& resp) {
        // 处理响应
        ESP_LOGI(TAG, "回复: %s", resp.text.c_str());
        ESP_LOGI(TAG, "会话ID: %s", resp.session_id.c_str());
    },
    [](const std::string& error) {
        // 处理错误
        ESP_LOGE(TAG, "错误: %s", error.c_str());
    }
);
```

#### 多轮对话

```cpp
// 第一轮
client.SendPrompt("我喜欢吃面食", ...);

// 第二轮(自动使用上一轮的 session_id)
client.SendPrompt("推荐一些美食", ...);
// 会根据第一轮的上下文推荐面食类美食
```

#### 清除会话

```cpp
// 清除会话,重新开始
client.ClearSession();
```

---

## 核心功能支持

### ✅ 已支持

| 功能 | 说明 |
|------|------|
| 单轮对话 | 基本问答 |
| 多轮对话 | 自动管理 session_id |
| 流式输出 | 实时返回内容 |
| 增量输出 | 只返回新增部分 |
| 深度思考 | DeepSeek-R1 等模型 |
| 错误处理 | HTTP 错误码处理 |

### 🔄 计划支持

| 功能 | 说明 | 预计时间 |
|------|------|----------|
| 实时语音流 | 直接传输音频 | Q1 2026 |
| 知识库检索 | RAG 能力 | Q1 2026 |
| 文件上传 | 图片/文档分析 | Q2 2026 |
| 长期记忆 | memory_id 管理 | Q2 2026 |
| 自定义插件 | MCP 工具调用 | Q2 2026 |

---

## API 参数说明

### 请求参数

```json
{
  "prompt": "用户输入的文本",
  "session_id": "会话ID(可选,用于多轮对话)",
  "memory_id": "记忆ID(可选,用于长期记忆)",
  "parameters": {
    "stream": true,
    "incremental_output": true,
    "has_thoughts": false,
    "enable_thinking": false
  },
  "biz_params": {
    "user_prompt_params": {},
    "user_defined_params": {},
    "user_defined_tokens": {}
  }
}
```

### 响应格式

**流式响应 (SSE):**

```
data: {"output":{"text":"我是","session_id":"xxx"}}

data: {"output":{"text":"通义千问","session_id":"xxx"}}

data: {"output":{"text":"","finish_reason":"stop","session_id":"xxx"}}

data: [DONE]
```

**非流式响应:**

```json
{
  "output": {
    "text": "我是通义千问,一个由阿里云开发的AI助手",
    "finish_reason": "stop",
    "session_id": "xxx",
    "thoughts": null,
    "doc_references": null
  },
  "usage": {
    "total_tokens": 100,
    "input_tokens": 20,
    "output_tokens": 80
  }
}
```

---

## ASR/TTS 集成

由于百炼 Application API 主要处理文本,需要额外集成语音服务:

### 方案一: 阿里云语音服务 (推荐)

**ASR (语音识别):**
- API: https://help.aliyun.com/zh/isi/developer-reference/real-time-speech-recognition
- 流程: 设备音频 → Opus编码 → ASR API → 文本

**TTS (语音合成):**
- API: https://help.aliyun.com/zh/isi/developer-reference/speech-synthesis
- 流程: 文本 → TTS API → 音频 → 设备播放

### 方案二: 使用原有服务器

保留原有服务器的 ASR/TTS 功能,只替换 LLM 部分:

```
设备 ──音频──> 服务器 ──ASR──> 文本
                ↓
              百炼API
                ↓
设备 <──音频── 服务器 <──TTS── 文本
```

---

## 常见问题

### Q1: 为什么要从官方服务器切换到百炼 API?

**A:** 主要优势:
- ✅ 无需自建服务器,降低维护成本
- ✅ 直接使用官方 API,稳定性更高
- ✅ 配置更简单,只需 API Key 和 App ID
- ✅ 支持更多高级功能(知识库、插件等)

### Q2: 切换后还能用原来的服务器吗?

**A:** 可以! 通过 menuconfig 随时切换:
```
Xiaozhi Assistant → API Mode
  (X) XiaoZhi Official Server  <-- 切回原来的
  ( ) Aliyun Bailian API
  ( ) Custom Server
```

### Q3: API 费用如何?

**A:** 阿里云百炼按量计费:
- 免费额度: 新用户有免费试用额度
- 计费方式: 按 Token 数量计费
- 详细定价: https://help.aliyun.com/zh/model-studio/product-overview/billing
- 建议: 先用免费额度测试,再评估成本

### Q4: 如何查看 API 调用日志?

**A:** 查看串口输出:
```bash
idf.py monitor

# 日志示例
I (12345) BailianApiClient: Initialized with App ID: xxx
I (12346) BailianApiClient: Sending request to: https://...
I (12500) BailianApiClient: Received response: 你好!我是通义千问...
```

### Q5: 支持哪些大模型?

**A:** 百炼平台支持的所有模型:
- ✅ Qwen 系列 (通义千问)
- ✅ Qwen3 (支持深度思考)
- ✅ DeepSeek-R1 (深度思考模型)
- ✅ 其他开源模型

在百炼控制台创建应用时选择模型即可。

### Q6: 多轮对话如何实现?

**A:** 自动管理,无需手动处理:
```cpp
// 第一次对话
client.SendPrompt("我喜欢看科幻电影", ...);
// SDK 自动保存 session_id

// 第二次对话
client.SendPrompt("推荐几部给我", ...);
// SDK 自动使用上次的 session_id
// API 会记住"科幻电影"的上下文
```

### Q7: 网络不好时会怎样?

**A:** 内置错误处理:
- 连接超时: 自动重试
- 请求失败: 返回错误回调
- 断网: 设备会显示网络错误提示

### Q8: 可以离线使用吗?

**A:** 不可以。百炼 API 需要网络连接。
如需离线功能,建议:
- 保留原有本地唤醒词
- 部分功能使用本地模型
- 或使用混合模式(在线+离线)

### Q9: API Key 安全吗?

**A:** 安全措施:
- ✅ 存储在 NVS 加密分区
- ✅ 不会出现在日志中
- ✅ 通过 HTTPS 加密传输
- ⚠️ 建议: 定期更换 API Key

### Q10: 如何调试?

**A:** 调试步骤:
```bash
# 1. 开启调试日志
idf.py menuconfig
  → Component config
    → Log output
      → Default log verbosity (Debug)

# 2. 监控输出
idf.py monitor

# 3. 查看详细请求
# 日志会显示:
# - 请求 URL
# - 请求体
# - 响应状态码
# - 响应内容
```

---

## 配置工具

### Web 配置页面

项目包含 Web 配置界面:

```bash
# 1. 设备连接 WiFi
# 2. 浏览器访问设备 IP
# 3. 进入 "百炼配置" 页面
# 4. 输入 API Key 和 App ID
# 5. 点击保存
```

### 串口配置工具

使用 Python 脚本:

```bash
# 安装依赖
pip install pyserial

# 运行配置脚本
python scripts/configure_bailian.py COM3 sk-xxx your-app-id

# 参数说明:
# COM3       - 串口号
# sk-xxx     - API Key
# your-app-id - 应用 ID
```

---

## 性能对比

| 指标 | 原有架构 | 百炼 API |
|------|---------|----------|
| 延迟 | ~500ms | ~300ms |
| 稳定性 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| 维护成本 | 高(需维护服务器) | 低(官方维护) |
| 费用 | 服务器+API | 仅API |
| 功能扩展 | 需自行开发 | 官方持续更新 |
| 定制性 | 高 | 中 |

---

## 迁移检查清单

- [ ] 获取阿里云账号
- [ ] 创建 API Key
- [ ] 创建百炼应用
- [ ] 配置 menuconfig
- [ ] 编译固件
- [ ] 烧录测试
- [ ] 配置 API Key
- [ ] 测试基本对话
- [ ] 测试多轮对话
- [ ] 测试错误处理
- [ ] 性能测试
- [ ] 生产部署

---

## 技术支持

### 获取帮助

1. **查看文档**
   - [阿里云百炼文档](https://help.aliyun.com/zh/model-studio/)
   - [项目 Wiki](https://github.com/78/xiaozhi-esp32/wiki)

2. **社区支持**
   - QQ群: 1011329060
   - GitHub Issues: https://github.com/78/xiaozhi-esp32/issues

3. **提交问题**
   - 描述问题现象
   - 附带日志输出
   - 说明配置信息

### 贡献代码

欢迎提交 Pull Request:
- 代码符合 Google C++ 规范
- 添加必要的注释
- 提供测试用例
- 更新相关文档

---

## 更新日志

### v2.2.0 (2026-01-19)
- ✨ 新增阿里云百炼 API 支持
- ✨ 新增流式输出功能
- ✨ 新增多轮对话自动管理
- 📝 完善文档和示例代码

### 未来计划
- 🚀 实时语音流支持
- 🚀 知识库集成
- 🚀 文件上传功能
- 🚀 长期记忆功能
- 🚀 MCP 工具调用

---

## 总结

本指南提供了从当前架构迁移到阿里云百炼 API 的完整方案:

✅ **简化配置** - 只需 API Key + App ID  
✅ **保持兼容** - 随时切换回原架构  
✅ **功能增强** - 支持更多高级特性  
✅ **降低成本** - 无需维护服务器  
✅ **易于扩展** - 官方持续更新  

开始使用阿里云百炼,让你的小智 AI 更智能! 🚀

---

*文档版本: 1.0*  
*最后更新: 2026-01-19*  
*作者: XiaoZhi Team*

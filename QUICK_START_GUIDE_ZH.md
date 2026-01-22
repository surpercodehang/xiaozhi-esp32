# 快速开始指南 - 阿里云百炼版本

## 📋 目录

1. [准备工作](#准备工作)
2. [配置步骤](#配置步骤)
3. [编译烧录](#编译烧录)
4. [测试验证](#测试验证)
5. [常见问题](#常见问题)

## 🛠️ 准备工作

### 硬件要求

- ESP32-S3 开发板 (推荐,或其他支持的板子)
- 麦克风 (用于语音输入)
- 扬声器 (用于语音输出)
- USB 数据线
- Wi-Fi 网络

### 软件要求

- ESP-IDF 5.4 或更高版本
- Python 3.8+
- Git
- Cursor 或 VSCode (带 ESP-IDF 插件)

### 阿里云账号准备

1. 注册阿里云账号: https://www.aliyun.com/
2. 开通百炼服务: https://bailian.console.aliyun.com/
3. 创建 API Key
4. 创建应用并获取 App ID

## ⚙️ 配置步骤

### 步骤 1: 克隆代码

```bash
git clone <本仓库地址>
cd xiaozhi-esp32
```

### 步骤 2: 配置阿里云密钥

**方法 A: 修改代码 (最简单)**

编辑文件 `main/protocols/dashscope_protocol.cc`,找到第 55-60 行:

```cpp
if (api_key_.empty()) {
    api_key_ = "sk-3c373c69431947e781a68fd9dad86ccd"; // 改为你的 API Key
}
if (app_id_.empty()) {
    app_id_ = "c897a3567b374b839bb0b1974aebc548"; // 改为你的 App ID
}
```

替换成你自己的密钥。

**方法 B: 使用 NVS 配置 (推荐生产环境)**

1. 创建文件 `nvs_dashscope.csv`:

```csv
key,type,encoding,value
dashscope,namespace,,
api_key,data,string,sk-你的实际密钥
app_id,data,string,你的实际应用ID
```

2. 生成 NVS 二进制文件:

```bash
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
    generate nvs_dashscope.csv nvs_dashscope.bin 0x6000
```

3. 烧录 NVS 分区:

```bash
esptool.py --port /dev/ttyUSB0 write_flash 0x9000 nvs_dashscope.bin
```

### 步骤 3: 配置开发板

```bash
idf.py menuconfig
```

关键配置项:

```
Board Configuration
  ├─ Board Type: 选择你的开发板型号
  └─ Audio Configuration
      ├─ Enable Audio Input: Yes
      ├─ Enable Audio Output: Yes
      └─ Sample Rate: 16000 Hz

Wi-Fi Configuration
  ├─ Wi-Fi SSID: 你的 Wi-Fi 名称
  └─ Wi-Fi Password: 你的 Wi-Fi 密码
```

保存并退出。

## 🔨 编译烧录

### 完整烧录 (首次使用)

```bash
# 清理之前的构建
idf.py fullclean

# 编译
idf.py build

# 烧录所有分区 (包括 bootloader、分区表、固件)
idf.py -p /dev/ttyUSB0 flash

# 监控串口输出
idf.py -p /dev/ttyUSB0 monitor
```

**Windows 用户**: 将 `/dev/ttyUSB0` 替换为 `COM3` 等实际端口号。

### 快速更新 (仅更新应用固件)

```bash
idf.py app-flash monitor
```

### 查看日志

```bash
idf.py -p /dev/ttyUSB0 monitor
```

退出监控: `Ctrl + ]`

## ✅ 测试验证

### 第一次启动

设备启动后,你应该看到类似的日志:

```
I (1234) Application: Initializing...
I (1250) Application: Network connecting...
I (3000) Application: Network connected
I (3100) Application: Skipping official server version check - using Dashscope protocol
I (3150) Dashscope: Dashscope protocol started with app_id: c897a3567b374b839bb0b1974aebc548
I (3200) Application: Activation done
I (3250) Application: Device ready
```

### 测试语音交互

1. **唤醒设备**
   - 说出唤醒词 (默认: "你好小智")
   - LED 应该亮起或改变颜色
   - 显示屏显示 "正在听..."

2. **说出指令**
   - 例如: "今天天气怎么样?"
   - 设备应该:
     - 显示识别的文本
     - 显示 "正在思考..."
     - 播放语音回复

3. **查看日志输出**
   ```
   I (5000) Dashscope: Audio channel opened with session: session_1234567890
   I (6000) Dashscope: Sending 32000 bytes to ASR
   I (7000) Dashscope: ASR Result: 今天天气怎么样
   I (7100) Dashscope: Sending to LLM: 今天天气怎么样
   I (8000) Dashscope: LLM Response: 今天天气晴朗...
   I (8100) Dashscope: Requesting TTS for: 今天天气晴朗...
   ```

### 测试清单

- [ ] 设备能正常启动
- [ ] Wi-Fi 连接成功
- [ ] 唤醒词检测正常
- [ ] 能够录音 (LED 指示)
- [ ] 语音识别有结果
- [ ] 大模型回复正常
- [ ] 语音播放正常
- [ ] 多轮对话上下文保持

## 🐛 常见问题

### 问题 1: 编译失败

**错误**: `fatal error: dashscope_protocol.h: No such file or directory`

**解决**:
```bash
# 确保文件存在
ls main/protocols/dashscope_protocol.h
ls main/protocols/dashscope_protocol.cc

# 重新编译
idf.py fullclean
idf.py build
```

### 问题 2: 设备无法连接 Wi-Fi

**解决**:
1. 检查 Wi-Fi 配置是否正确
2. 确认 Wi-Fi 信号强度
3. 查看日志: `I (xxx) wifi: ...`
4. 尝试重启设备

### 问题 3: API 认证失败

**日志**: `HTTP POST request failed` 或 `Status = 401`

**解决**:
1. 验证 API Key 是否正确
2. 检查 API Key 是否过期
3. 确认 App ID 是否正确
4. 登录阿里云控制台检查服务状态

### 问题 4: 无法唤醒

**解决**:
1. 检查麦克风连接
2. 调整麦克风增益
3. 在安静环境测试
4. 查看日志: `I (xxx) AudioService: Wake word detected`

### 问题 5: 听不到回复

**解决**:
1. 检查扬声器连接
2. 调整音量
3. 验证音频输出配置
4. 查看 TTS 相关日志

### 问题 6: 内存不足

**日志**: `E (xxx) Application: Failed to allocate memory`

**解决**:
1. 使用 PSRAM (ESP32-S3)
   ```
   Component config → ESP32S3-Specific → Support for external, SPI-connected RAM
   ```
2. 减小音频缓冲区大小
3. 优化任务栈大小

### 问题 7: 响应很慢

**原因分析**:
- 网络延迟
- API 限流
- 音频编解码耗时

**解决**:
1. 检查网络速度
2. 升级阿里云服务等级
3. 启用硬件加速 (如果支持)
4. 优化音频处理流程

## 📊 性能优化

### 降低延迟

1. **使用流式 ASR**
   - 实时发送音频,无需等待完整录音

2. **使用流式 TTS**
   - 边接收边播放,降低响应时间

3. **网络优化**
   - 使用 WebSocket 保持长连接
   - 启用 HTTP/2

### 提高识别准确率

1. **音频质量**
   - 使用高质量麦克风
   - 添加降噪处理
   - 启用回声消除 (AEC)

2. **环境优化**
   - 在安静环境使用
   - 保持适当的说话距离
   - 清晰发音

### 节省流量

1. **使用更高的压缩率**
   - 调整 Opus 编码参数
   - 降低采样率 (如果可接受)

2. **减少不必要的请求**
   - 实现本地 VAD
   - 过滤静音片段

## 📝 日志分析

### 关键日志标签

```bash
# 查看协议相关日志
idf.py monitor | grep "Dashscope"

# 查看音频处理日志
idf.py monitor | grep "AudioService"

# 查看应用状态日志
idf.py monitor | grep "Application"

# 查看网络日志
idf.py monitor | grep "wifi"
```

### 性能监控

```bash
# 查看内存使用
I (xxx) Application: Free heap: 123456 bytes

# 查看任务状态
I (xxx) Application: Task stack high water mark: 1234
```

## 🔧 高级配置

### 自定义唤醒词

编辑 `main/audio/audio_service.cc`,修改唤醒词配置。

### 调整音频参数

编辑 `main/protocols/dashscope_protocol.cc`:

```cpp
const size_t MIN_AUDIO_SIZE = 16000 * 2; // 1秒音频 → 调整为 0.5 秒
```

### 修改 LLM 参数

在 `SendToLlm` 函数中添加更多参数:

```cpp
cJSON_AddNumberToObject(parameters, "temperature", 0.8);
cJSON_AddNumberToObject(parameters, "top_p", 0.9);
cJSON_AddNumberToObject(parameters, "max_tokens", 2000);
```

## 📚 相关文档

- [详细集成说明](DASHSCOPE_INTEGRATION.md)
- [配置说明](dashscope_config.md)
- [原项目 README](README_zh.md)
- [阿里云百炼文档](https://help.aliyun.com/zh/model-studio/)

## 🤝 贡献

欢迎提交 Issue 和 Pull Request!

## 📄 许可证

MIT License - 详见 LICENSE 文件

---

**祝你使用愉快! 🎉**

如有问题,请查看文档或提交 Issue。

# 阿里云百炼配置说明

本项目已修改为直接使用阿里云百炼应用,无需连接官方服务器。

## 配置方法

### 方法1: 通过 NVS 配置 (推荐)

使用 ESP-IDF 的 NVS 分区工具设置以下配置:

**命名空间**: `dashscope`

| 键名 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| api_key | string | 阿里云 API Key | sk-3c373c69431947e781a68fd9dad86ccd |
| app_id | string | 百炼应用 ID | c897a3567b374b839bb0b1974aebc548 |

### 方法2: 修改代码中的默认值

编辑文件: `main/protocols/dashscope_protocol.cc`

找到以下代码并修改:

```cpp
if (api_key_.empty()) {
    api_key_ = "你的API_KEY"; // 修改这里
}
if (app_id_.empty()) {
    app_id_ = "你的APP_ID"; // 修改这里
}
```

## 使用 NVS 分区工具配置

### 1. 创建 NVS 分区 CSV 文件

创建文件 `nvs_dashscope.csv`:

```csv
key,type,encoding,value
dashscope,namespace,,
api_key,data,string,sk-3c373c69431947e781a68fd9dad86ccd
app_id,data,string,c897a3567b374b839bb0b1974aebc548
```

### 2. 生成 NVS 分区二进制文件

```bash
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate nvs_dashscope.csv nvs_dashscope.bin 0x6000
```

### 3. 烧录 NVS 分区

```bash
esptool.py --port /dev/ttyUSB0 write_flash 0x9000 nvs_dashscope.bin
```

注意: 分区地址 `0x9000` 需要根据你的分区表配置调整。

## API 配置说明

### 获取阿里云 API Key

1. 登录 [阿里云百炼控制台](https://bailian.console.aliyun.com/)
2. 进入 "API Key 管理"
3. 创建或获取你的 API Key

### 创建百炼应用

1. 在百炼控制台创建新应用
2. 配置应用的模型和参数
3. 获取应用 ID (app_id)

## 功能说明

本项目集成了以下阿里云服务:

1. **语音识别 (ASR)**: 将用户语音转换为文本
2. **大模型对话 (LLM)**: 使用百炼应用处理对话
3. **语音合成 (TTS)**: 将回复文本转换为语音

## 编译和烧录

```bash
# 配置项目
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash monitor
```

## 注意事项

1. 确保设备已连接 Wi-Fi
2. API Key 和 App ID 必须正确配置
3. 阿里云账号需要有足够的余额或免费额度
4. 建议使用 HTTPS 连接以确保安全性

## 故障排除

### 无法连接到阿里云服务

- 检查网络连接
- 验证 API Key 和 App ID 是否正确
- 查看串口日志中的错误信息

### 语音识别不准确

- 确保麦克风正常工作
- 调整音量增益
- 在安静环境下测试

### 语音合成无声音

- 检查扬声器连接
- 调整音量设置
- 查看 TTS 服务是否正常返回数据

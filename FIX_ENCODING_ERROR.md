# 修复编码错误

## 问题原因

Windows 系统使用 GBK 编码,配置文件中的中文注释导致解析失败。

## 已修复

✅ 已将 `sdkconfig.defaults.esp32s3` 中的中文注释改为英文:
```ini
# 阿里云百炼 API 配置  →  # Aliyun Bailian API Configuration
```

## 编译步骤

### 方法一: 使用 ESP-IDF 命令提示符 (推荐)

1. 打开 **ESP-IDF 5.5 CMD** (开始菜单中搜索)

2. 切换到项目目录:
```cmd
cd C:\work\xiaozhi\xiaozhi-lemon\xiaozhi-esp32
```

3. 清理旧配置:
```cmd
idf.py fullclean
```

4. 编译:
```cmd
idf.py build
```

### 方法二: 使用 VSCode

1. 打开 VSCode

2. 按 `Ctrl+Shift+P` 打开命令面板

3. 输入并选择: **ESP-IDF: Build your project**

4. 等待编译完成

### 方法三: 手动删除构建目录

如果仍然有问题:

1. 删除 `build` 文件夹
```cmd
rmdir /s /q build
```

2. 删除 `sdkconfig` 文件
```cmd
del sdkconfig
```

3. 重新编译
```cmd
idf.py build
```

## 预期输出

编译成功后应该看到:
```
Project build complete. To flash, run:
 idf.py -p (PORT) flash
or
 idf.py -p (PORT) flash monitor
```

## 如果还有编码错误

检查以下文件是否有中文:
- `sdkconfig.defaults`
- `sdkconfig.defaults.esp32s3`
- `sdkconfig` (如果存在)

确保文件编码为 **UTF-8 without BOM** 或纯 ASCII。

### 在 VSCode 中转换编码:

1. 打开文件
2. 点击右下角的编码 (例如: GBK)
3. 选择 "Save with Encoding"
4. 选择 "UTF-8"

## 配置说明

当前配置的 API 信息:
- API Key: `sk-3c373c69431947e781a68fd9dad86ccd`
- App ID: `c897a3567b374b839bb0b1974aebc548`
- 模式: 阿里云百炼 API (包含 STT + LLM + TTS)

## 下一步

编译成功后:

1. **烧录固件**:
```cmd
idf.py -p COM3 flash
```

2. **查看日志**:
```cmd
idf.py -p COM3 monitor
```

3. **测试功能**:
   - 唤醒设备
   - 说话
   - 查看日志中的 ASR/LLM/TTS 输出

## 常见问题

### Q1: 找不到 idf.py 命令

**A:** 请使用 ESP-IDF 命令提示符,或在 PowerShell 中运行:
```powershell
. $env:IDF_PATH/export.ps1
```

### Q2: 编译时内存不足

**A:** 关闭其他程序,或增加虚拟内存:
```
控制面板 → 系统 → 高级系统设置 → 性能设置 → 高级 → 虚拟内存
```

### Q3: COM 口找不到

**A:** 检查设备管理器中的端口号,或使用:
```cmd
idf.py -p COM1 flash
idf.py -p COM2 flash
...
```

---

**现在可以开始编译了!** 🚀

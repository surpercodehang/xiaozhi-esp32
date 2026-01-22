# 阿里云百炼版本 - 文件索引

## 📚 文档导航

本文档帮助你快速找到需要的文件和信息。

## 🎯 快速开始

如果你是第一次使用,请按顺序阅读:

1. **[README_DASHSCOPE_ZH.md](README_DASHSCOPE_ZH.md)** - 项目概览
2. **[QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md)** - 快速开始指南
3. **[dashscope_config.md](dashscope_config.md)** - 配置说明

## 📁 文件分类

### 一、用户文档 (开始使用)

| 文件名 | 说明 | 适合人群 | 必读 |
|-------|------|---------|------|
| [README_DASHSCOPE_ZH.md](README_DASHSCOPE_ZH.md) | 项目概览和简介 | 所有用户 | ⭐⭐⭐ |
| [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) | 从零开始的完整教程 | 新手 | ⭐⭐⭐ |
| [dashscope_config.md](dashscope_config.md) | 配置方法详解 | 所有用户 | ⭐⭐⭐ |

### 二、技术文档 (深入理解)

| 文件名 | 说明 | 适合人群 | 必读 |
|-------|------|---------|------|
| [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) | 详细的技术实现文档 | 开发者 | ⭐⭐ |
| [CODE_CHANGES_SUMMARY_ZH.md](CODE_CHANGES_SUMMARY_ZH.md) | 代码变更详细对比 | 开发者 | ⭐⭐ |
| [MIGRATION_SUMMARY_ZH.md](MIGRATION_SUMMARY_ZH.md) | 迁移总结报告 | 项目管理/开发者 | ⭐⭐ |

### 三、测试文档 (质量保证)

| 文件名 | 说明 | 适合人群 | 必读 |
|-------|------|---------|------|
| [TESTING_GUIDE_ZH.md](TESTING_GUIDE_ZH.md) | 完整的测试清单和方法 | 测试人员/开发者 | ⭐⭐ |

### 四、索引文档 (导航)

| 文件名 | 说明 | 适合人群 | 必读 |
|-------|------|---------|------|
| [FILES_INDEX_ZH.md](FILES_INDEX_ZH.md) | 本文件,文件导航 | 所有用户 | ⭐ |

### 五、核心代码 (技术实现)

| 文件路径 | 说明 | 行数 | 状态 |
|---------|------|------|------|
| `main/protocols/dashscope_protocol.h` | 协议头文件 | ~70 | 新增 |
| `main/protocols/dashscope_protocol.cc` | 协议实现 | ~450 | 新增 |
| `main/application.cc` | 应用主逻辑 | 修改 | 已修改 |
| `main/CMakeLists.txt` | 编译配置 | +1 | 已修改 |

### 六、原项目文档

| 文件名 | 说明 |
|-------|------|
| [README_zh.md](README_zh.md) | 原项目中文说明 |
| [README.md](README.md) | 原项目英文说明 |
| [docs/](docs/) | 原项目文档目录 |

## 🗺️ 按使用场景导航

### 场景 1: 我是新手,第一次使用

**阅读顺序**:

1. [README_DASHSCOPE_ZH.md](README_DASHSCOPE_ZH.md) - 了解项目
2. [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 开始配置
3. [dashscope_config.md](dashscope_config.md) - 配置参考

**时间**: 约 30 分钟

---

### 场景 2: 我要开发/修改代码

**阅读顺序**:

1. [README_DASHSCOPE_ZH.md](README_DASHSCOPE_ZH.md) - 项目概览
2. [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) - 技术细节
3. [CODE_CHANGES_SUMMARY_ZH.md](CODE_CHANGES_SUMMARY_ZH.md) - 代码变更
4. 查看源代码:
   - `main/protocols/dashscope_protocol.h`
   - `main/protocols/dashscope_protocol.cc`
   - `main/application.cc`

**时间**: 约 1-2 小时

---

### 场景 3: 我要测试功能

**阅读顺序**:

1. [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 快速配置
2. [TESTING_GUIDE_ZH.md](TESTING_GUIDE_ZH.md) - 测试清单

**时间**: 约 1 小时 (含测试时间)

---

### 场景 4: 我遇到问题需要排查

**查找顺序**:

1. [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 查看"常见问题"章节
2. [TESTING_GUIDE_ZH.md](TESTING_GUIDE_ZH.md) - 查看"调试技巧"章节
3. [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) - 查看"故障排除"章节
4. GitHub Issues

---

### 场景 5: 我要做项目评审/管理

**阅读顺序**:

1. [MIGRATION_SUMMARY_ZH.md](MIGRATION_SUMMARY_ZH.md) - 项目总结
2. [CODE_CHANGES_SUMMARY_ZH.md](CODE_CHANGES_SUMMARY_ZH.md) - 技术变更
3. [TESTING_GUIDE_ZH.md](TESTING_GUIDE_ZH.md) - 测试覆盖

**时间**: 约 1 小时

---

## 📖 文档详细说明

### README_DASHSCOPE_ZH.md

**内容**:
- 项目简介
- 与原版对比
- 快速开始
- 架构说明
- 性能指标
- 常见问题

**阅读时长**: 10 分钟

**何时阅读**: 首次了解项目

---

### QUICK_START_GUIDE_ZH.md

**内容**:
- 准备工作清单
- 详细配置步骤
- 编译烧录指南
- 测试验证方法
- 常见问题解答
- 性能优化建议

**阅读时长**: 20 分钟

**何时阅读**: 准备开始使用

---

### dashscope_config.md

**内容**:
- 配置方法 (代码/NVS/menuconfig)
- API 密钥获取
- 分区工具使用
- 配置示例
- 故障排除

**阅读时长**: 15 分钟

**何时阅读**: 需要配置密钥时

---

### DASHSCOPE_INTEGRATION.md

**内容**:
- 架构设计详解
- 工作流程说明
- API 使用文档
- 音频处理细节
- 内存优化方案
- 调试日志说明
- 后续改进方向

**阅读时长**: 30-60 分钟

**何时阅读**: 需要深入了解技术实现

---

### CODE_CHANGES_SUMMARY_ZH.md

**内容**:
- 详细的代码对比
- 每个修改点的说明
- 架构差异分析
- 性能对比
- 代码质量评估

**阅读时长**: 30-45 分钟

**何时阅读**: 需要了解具体改动

---

### MIGRATION_SUMMARY_ZH.md

**内容**:
- 项目目标和完成状态
- 文件清单
- 架构改造说明
- 核心功能介绍
- 性能指标
- 改进建议
- 版本历史

**阅读时长**: 20-30 分钟

**何时阅读**: 需要全局了解项目

---

### TESTING_GUIDE_ZH.md

**内容**:
- 测试环境准备
- 6 个阶段的完整测试
- 测试报告模板
- 调试技巧
- 完成标准

**阅读时长**: 30 分钟 (不含实际测试)

**何时阅读**: 准备测试功能

---

## 🔍 按主题查找

### 主题: 配置相关

- [dashscope_config.md](dashscope_config.md) - 主要配置文档
- [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 第 2 章"配置步骤"
- [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) - "配置方法"章节

### 主题: 编译烧录

- [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 第 3 章"编译烧录"
- [dashscope_config.md](dashscope_config.md) - "编译和烧录"章节

### 主题: 测试验证

- [TESTING_GUIDE_ZH.md](TESTING_GUIDE_ZH.md) - 完整测试指南
- [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 第 4 章"测试验证"

### 主题: 故障排查

- [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 第 5 章"常见问题"
- [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) - "故障排除"章节
- [TESTING_GUIDE_ZH.md](TESTING_GUIDE_ZH.md) - "调试技巧"章节

### 主题: 性能优化

- [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) - "性能优化建议"章节
- [QUICK_START_GUIDE_ZH.md](QUICK_START_GUIDE_ZH.md) - 第 6 章"性能优化"
- [CODE_CHANGES_SUMMARY_ZH.md](CODE_CHANGES_SUMMARY_ZH.md) - "性能指标"章节

### 主题: 架构设计

- [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) - "架构说明"章节
- [CODE_CHANGES_SUMMARY_ZH.md](CODE_CHANGES_SUMMARY_ZH.md) - "架构改造"章节
- [MIGRATION_SUMMARY_ZH.md](MIGRATION_SUMMARY_ZH.md) - "架构改造"章节

### 主题: API 使用

- [DASHSCOPE_INTEGRATION.md](DASHSCOPE_INTEGRATION.md) - "API 使用说明"章节
- [CODE_CHANGES_SUMMARY_ZH.md](CODE_CHANGES_SUMMARY_ZH.md) - 查看代码示例

### 主题: 代码实现

- [CODE_CHANGES_SUMMARY_ZH.md](CODE_CHANGES_SUMMARY_ZH.md) - 详细代码对比
- `main/protocols/dashscope_protocol.h` - 头文件
- `main/protocols/dashscope_protocol.cc` - 实现文件

## 📝 文档更新日志

| 日期 | 文件 | 变更 |
|-----|------|------|
| 2026-01-21 | 所有文件 | 初始版本创建 |

## 🎯 推荐阅读路径

### 路径 1: 快速上手 (30分钟)

```
README_DASHSCOPE_ZH.md (5分钟)
    ↓
QUICK_START_GUIDE_ZH.md (20分钟)
    ↓
开始动手配置
```

### 路径 2: 深入了解 (2小时)

```
README_DASHSCOPE_ZH.md (5分钟)
    ↓
QUICK_START_GUIDE_ZH.md (20分钟)
    ↓
DASHSCOPE_INTEGRATION.md (45分钟)
    ↓
CODE_CHANGES_SUMMARY_ZH.md (30分钟)
    ↓
查看源代码 (30分钟)
```

### 路径 3: 全面掌握 (4小时)

```
所有用户文档 (35分钟)
    ↓
所有技术文档 (2小时)
    ↓
测试文档 (1小时)
    ↓
阅读源代码 (1小时)
```

## 🔗 外部资源

### 阿里云文档

- [百炼控制台](https://bailian.console.aliyun.com/)
- [百炼开发文档](https://help.aliyun.com/zh/model-studio/)
- [语音识别文档](https://help.aliyun.com/zh/isi/)
- [语音合成文档](https://help.aliyun.com/zh/tts/)

### ESP32 相关

- [ESP-IDF 文档](https://docs.espressif.com/projects/esp-idf/)
- [ESP32-S3 数据手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_cn.pdf)

### 原项目

- [小智 ESP32 GitHub](https://github.com/78/xiaozhi-esp32)
- [原项目 Wiki](https://ccnphfhqs21z.feishu.cn/wiki/F5krwD16viZoF0kKkvDcrZNYnhb)

## 📞 获取帮助

如果文档中找不到答案:

1. **GitHub Issues**: 提交新 Issue
2. **QQ 群**: 1011329060
3. **Discussions**: GitHub Discussions
4. **邮件**: (如有)

## 🤝 贡献文档

欢迎改进文档:

1. Fork 项目
2. 修改文档
3. 提交 Pull Request

## ✅ 文档检查清单

使用前请确认:

- [ ] 已阅读 README_DASHSCOPE_ZH.md
- [ ] 已阅读 QUICK_START_GUIDE_ZH.md
- [ ] 已配置 API Key 和 App ID
- [ ] 已准备好硬件设备
- [ ] 已安装 ESP-IDF 5.4+

---

**文档索引最后更新**: 2026-01-21

**维护者**: 小智社区

**反馈**: 欢迎通过 Issue 提出文档改进建议

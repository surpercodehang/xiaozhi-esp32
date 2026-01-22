@echo off
REM Windows 环境依赖安装脚本

echo ╔═══════════════════════════════════════════════════════╗
echo ║     小智百炼服务器 - Windows 依赖安装                 ║
echo ╚═══════════════════════════════════════════════════════╝
echo.

echo [1/5] 检查 Python 版本...
python --version
if %errorlevel% neq 0 (
    echo 错误: 未安装 Python 或 Python 不在 PATH 中
    echo 请从 https://www.python.org/downloads/ 下载安装 Python 3.8+
    pause
    exit /b 1
)
echo.

echo [2/5] 升级 pip...
python -m pip install --upgrade pip
echo.

echo [3/5] 安装基础依赖...
pip install websockets dashscope
echo.

echo [4/5] 安装 Opus 音频库...
echo 注意: Windows 下 opuslib 安装可能需要额外步骤
pip install opuslib
if %errorlevel% neq 0 (
    echo.
    echo ⚠️  opuslib 安装失败，这是正常的！
    echo    可以先跳过音频功能，使用简化版测试
    echo.
)
echo.

echo [5/5] 安装设备配置工具...
pip install esptool esp-idf-nvs-partition-gen
echo.

echo ╔═══════════════════════════════════════════════════════╗
echo ║     安装完成！                                         ║
echo ╚═══════════════════════════════════════════════════════╝
echo.
echo 下一步:
echo   1. 编辑 simple_bailian_test.py 填入 API Key 和 App ID
echo   2. 运行: python simple_bailian_test.py
echo   3. 配置设备连接
echo.
pause

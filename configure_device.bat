@echo off
REM 配置 ESP32 设备连接到自定义服务器 (Windows 版本)

setlocal enabledelayedexpansion

echo ╔═══════════════════════════════════════════════════════╗
echo ║     小智 ESP32 设备配置工具 (Windows)                 ║
echo ╚═══════════════════════════════════════════════════════╝
echo.

REM 检查参数
if "%~1"=="" goto usage
if "%~2"=="" goto usage

set PORT=%~1
set SERVER_URL=%~2

echo 配置信息:
echo   串口: %PORT%
echo   服务器: %SERVER_URL%
echo.

REM 检查 esptool 是否安装
where esptool.py >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: esptool.py 未安装
    echo 请运行: pip install esptool
    exit /b 1
)

REM 检查 nvs_partition_gen.py 是否安装
where nvs_partition_gen.py >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: nvs_partition_gen.py 未安装
    echo 请运行: pip install esp-idf-nvs-partition-gen
    exit /b 1
)

REM 创建临时目录
set TEMP_DIR=%TEMP%\xiaozhi_config_%RANDOM%
mkdir "%TEMP_DIR%"
echo 创建临时目录: %TEMP_DIR%

REM 创建 NVS 配置文件
set NVS_CSV=%TEMP_DIR%\nvs_config.csv
echo key,type,encoding,value > "%NVS_CSV%"
echo websocket,namespace,, >> "%NVS_CSV%"
echo url,data,string,%SERVER_URL% >> "%NVS_CSV%"
echo token,data,string, >> "%NVS_CSV%"
echo version,data,u32,1 >> "%NVS_CSV%"

echo 生成 NVS 配置文件:
type "%NVS_CSV%"
echo.

REM 生成 NVS 分区
set NVS_BIN=%TEMP_DIR%\nvs.bin
echo 生成 NVS 分区...
nvs_partition_gen.py generate "%NVS_CSV%" "%NVS_BIN%" 0x6000

if not exist "%NVS_BIN%" (
    echo 错误: NVS 分区生成失败
    exit /b 1
)

echo ✓ NVS 分区生成成功
echo.

REM 烧录到设备
echo 烧录到设备...
echo   串口: %PORT%
echo   地址: 0x9000
echo.

esptool.py --port %PORT% write_flash 0x9000 "%NVS_BIN%"

if %errorlevel% equ 0 (
    echo.
    echo ╔═══════════════════════════════════════════════════════╗
    echo ║     配置成功！                                         ║
    echo ╚═══════════════════════════════════════════════════════╝
    echo.
    echo 下一步:
    echo   1. 重启设备
    echo   2. 设备将自动连接到: %SERVER_URL%
    echo   3. 查看服务器日志确认连接
    echo.
) else (
    echo 错误: 烧录失败
    exit /b 1
)

REM 清理临时文件
rmdir /s /q "%TEMP_DIR%"
echo 清理临时文件
goto end

:usage
echo 使用方法:
echo   %~nx0 ^<串口^> ^<服务器地址^>
echo.
echo 示例:
echo   %~nx0 COM3 ws://192.168.1.100:8765
echo.
exit /b 1

:end
endlocal

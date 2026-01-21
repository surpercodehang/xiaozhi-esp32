#!/bin/bash
# 配置 ESP32 设备连接到自定义服务器

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}╔═══════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     小智 ESP32 设备配置工具                           ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════════════════════╝${NC}"
echo ""

# 检查参数
if [ $# -lt 2 ]; then
    echo -e "${YELLOW}使用方法:${NC}"
    echo "  $0 <串口> <服务器地址>"
    echo ""
    echo -e "${YELLOW}示例:${NC}"
    echo "  $0 /dev/ttyUSB0 ws://192.168.1.100:8765"
    echo "  $0 COM3 ws://192.168.1.100:8765"
    echo ""
    exit 1
fi

PORT=$1
SERVER_URL=$2

echo -e "${YELLOW}配置信息:${NC}"
echo "  串口: $PORT"
echo "  服务器: $SERVER_URL"
echo ""

# 检查 esptool 是否安装
if ! command -v esptool.py &> /dev/null; then
    echo -e "${RED}错误: esptool.py 未安装${NC}"
    echo "请运行: pip install esptool"
    exit 1
fi

# 检查 esp-idf-nvs-partition-gen 是否安装
if ! command -v nvs_partition_gen.py &> /dev/null; then
    echo -e "${RED}错误: nvs_partition_gen.py 未安装${NC}"
    echo "请运行: pip install esp-idf-nvs-partition-gen"
    exit 1
fi

# 创建临时目录
TEMP_DIR=$(mktemp -d)
echo -e "${GREEN}创建临时目录: $TEMP_DIR${NC}"

# 创建 NVS 配置文件
NVS_CSV="$TEMP_DIR/nvs_config.csv"
echo "key,type,encoding,value" > "$NVS_CSV"
echo "websocket,namespace,," >> "$NVS_CSV"
echo "url,data,string,$SERVER_URL" >> "$NVS_CSV"
echo "token,data,string," >> "$NVS_CSV"
echo "version,data,u32,1" >> "$NVS_CSV"

echo -e "${GREEN}生成 NVS 配置文件:${NC}"
cat "$NVS_CSV"
echo ""

# 生成 NVS 分区
NVS_BIN="$TEMP_DIR/nvs.bin"
echo -e "${YELLOW}生成 NVS 分区...${NC}"
nvs_partition_gen.py generate "$NVS_CSV" "$NVS_BIN" 0x6000

if [ ! -f "$NVS_BIN" ]; then
    echo -e "${RED}错误: NVS 分区生成失败${NC}"
    exit 1
fi

echo -e "${GREEN}✓ NVS 分区生成成功${NC}"
echo ""

# 烧录到设备
echo -e "${YELLOW}烧录到设备...${NC}"
echo "  串口: $PORT"
echo "  地址: 0x9000"
echo ""

esptool.py --port "$PORT" write_flash 0x9000 "$NVS_BIN"

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}╔═══════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║     配置成功！                                         ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}下一步:${NC}"
    echo "  1. 重启设备"
    echo "  2. 设备将自动连接到: $SERVER_URL"
    echo "  3. 查看服务器日志确认连接"
    echo ""
else
    echo -e "${RED}错误: 烧录失败${NC}"
    exit 1
fi

# 清理临时文件
rm -rf "$TEMP_DIR"
echo -e "${GREEN}清理临时文件${NC}"

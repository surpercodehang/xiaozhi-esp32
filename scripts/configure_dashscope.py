#!/usr/bin/env python3
"""
阿里云百炼协议配置工具

用于配置小智AI聊天机器人使用阿里云百炼协议
"""

import os
import sys
import argparse

def update_kconfig(api_key, app_id, base_url="https://dashscope.aliyuncs.com/api/v1/"):
    """更新Kconfig文件中的DashScope配置"""
    kconfig_path = "main/Kconfig.projbuild"

    if not os.path.exists(kconfig_path):
        print(f"错误: 找不到Kconfig文件 {kconfig_path}")
        return False

    try:
        with open(kconfig_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # 更新API Key
        if 'CONFIG_DASHSCOPE_API_KEY=' in content:
            content = content.replace(
                'CONFIG_DASHSCOPE_API_KEY=""',
                f'CONFIG_DASHSCOPE_API_KEY="{api_key}"'
            )
        else:
            print("警告: 找不到CONFIG_DASHSCOPE_API_KEY配置项")

        # 更新App ID
        if 'CONFIG_DASHSCOPE_APP_ID=' in content:
            content = content.replace(
                'CONFIG_DASHSCOPE_APP_ID=""',
                f'CONFIG_DASHSCOPE_APP_ID="{app_id}"'
            )
        else:
            print("警告: 找不到CONFIG_DASHSCOPE_APP_ID配置项")

        # 更新Base URL
        if 'CONFIG_DASHSCOPE_BASE_URL=' in content:
            content = content.replace(
                'CONFIG_DASHSCOPE_BASE_URL="https://dashscope.aliyuncs.com/api/v1/"',
                f'CONFIG_DASHSCOPE_BASE_URL="{base_url}"'
            )
        else:
            print("警告: 找不到CONFIG_DASHSCOPE_BASE_URL配置项")

        with open(kconfig_path, 'w', encoding='utf-8') as f:
            f.write(content)

        print("✅ Kconfig文件已更新")
        return True

    except Exception as e:
        print(f"错误: 更新Kconfig文件失败: {e}")
        return False

def create_sdkconfig_entries():
    """创建sdkconfig.defaults条目（如果不存在）"""
    sdkconfig_path = "sdkconfig.defaults"

    entries = [
        f'# DashScope Configuration',
        f'CONFIG_DASHSCOPE_API_KEY=""',
        f'CONFIG_DASHSCOPE_APP_ID=""',
        f'CONFIG_DASHSCOPE_BASE_URL="https://dashscope.aliyuncs.com/api/v1/"',
    ]

    try:
        if os.path.exists(sdkconfig_path):
            with open(sdkconfig_path, 'r', encoding='utf-8') as f:
                content = f.read()

            # 检查是否已存在DashScope配置
            if 'CONFIG_DASHSCOPE_API_KEY=' in content:
                print("sdkconfig.defaults已包含DashScope配置")
                return True
        else:
            content = ""

        # 添加配置条目
        if content and not content.endswith('\n'):
            content += '\n'

        content += '\n'.join(entries) + '\n'

        with open(sdkconfig_path, 'w', encoding='utf-8') as f:
            f.write(content)

        print("✅ sdkconfig.defaults已更新")
        return True

    except Exception as e:
        print(f"错误: 更新sdkconfig.defaults失败: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description='配置小智AI聊天机器人使用阿里云百炼协议',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  python configure_dashscope.py --api-key sk-xxx --app-id your-app-id
  python configure_dashscope.py --api-key sk-xxx --app-id your-app-id --base-url https://custom.endpoint.com/api/v1/
        """
    )

    parser.add_argument('--api-key', required=True,
                       help='阿里云百炼API Key')
    parser.add_argument('--app-id', required=True,
                       help='阿里云百炼应用ID')
    parser.add_argument('--base-url', default='https://dashscope.aliyuncs.com/api/v1/',
                       help='API基础URL (可选)')

    args = parser.parse_args()

    print("🚀 配置阿里云百炼协议...")
    print(f"API Key: {args.api_key[:8]}...")  # 只显示前8个字符
    print(f"App ID: {args.app_id}")
    print(f"Base URL: {args.base_url}")
    print()

    # 更新Kconfig
    if not update_kconfig(args.api_key, args.app_id, args.base_url):
        sys.exit(1)

    # 更新sdkconfig.defaults
    if not create_sdkconfig_entries():
        sys.exit(1)

    print()
    print("🎉 配置完成!")
    print()
    print("接下来请运行:")
    print("  idf.py menuconfig  # 选择 DashScope Protocol")
    print("  idf.py build")
    print("  idf.py flash")
    print()
    print("📖 更多信息请参考: docs/dashscope-protocol.md")

if __name__ == '__main__':
    main()
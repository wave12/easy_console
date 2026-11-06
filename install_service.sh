#!/bin/bash
# =====================================================
# nl_server_monitor systemd 服务安装脚本
# =====================================================

set -e # 遇到错误立即停止

# 1. 定义变量（请根据你的实际情况修改）
PROJECT_DIR="/home/sg/easy_console"
BINARY_PATH="${PROJECT_DIR}/build/easy_console"
CONFIG_PATH="${PROJECT_DIR}/config.json"
SERVICE_NAME="easy_console"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"

echo "=== 1. 检查程序文件和配置文件是否存在 ==="
if [ ! -f "$BINARY_PATH" ]; then
    echo "错误: 找不到可执行文件 $BINARY_PATH"
    echo "请先进入 build 目录执行 make 编译。"
    exit 1
fi

if [ ! -f "$CONFIG_PATH" ]; then
    echo "错误: 找不到配置文件 $CONFIG_PATH"
    exit 1
fi

echo "=== 2. 生成 systemd 服务配置文件 ==="
# 使用 cat 写入服务配置
sudo bash -c "cat > ${SERVICE_FILE}" <<EOF
[Unit]
Description=easy_console Agent
After=network.target

[Service]
Type=simple
# 运行程序的用户（建议不要用 root，这里使用你当前的用户 sg）
User=sg
Group=sg
# 程序所在目录
WorkingDirectory=${PROJECT_DIR}/build
# 启动命令
ExecStart=${BINARY_PATH}
# 进程崩溃后自动重启
Restart=always
RestartSec=5
# 标准输出和错误重定向到系统日志
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

echo "=== 3. 重新加载 systemd 配置 ==="
sudo systemctl daemon-reload

echo "=== 4. 设置开机自启并立即启动服务 ==="
sudo systemctl enable ${SERVICE_NAME}
sudo systemctl restart ${SERVICE_NAME}

echo "====================================================="
echo " 服务安装并启动成功！"
echo " 查看服务状态: sudo systemctl status ${SERVICE_NAME}"
echo " 查看实时日志: sudo journalctl -u ${SERVICE_NAME} -f"
echo " 停止服务:     sudo systemctl stop ${SERVICE_NAME}"
echo " 重启服务:     sudo systemctl restart ${SERVICE_NAME}"
echo "====================================================="
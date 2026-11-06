#!/bin/bash
# =====================================================
# MonitorAgent systemd 服务卸载脚本
# =====================================================

set -e

SERVICE_NAME="nl_server_monitor"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"

echo "=== 1. 停止服务 ==="
# 加上 || true 防止服务本来就没在运行时报错退出
sudo systemctl stop ${SERVICE_NAME} || true

echo "=== 2. 取消开机自启 ==="
sudo systemctl disable ${SERVICE_NAME} || true

echo "=== 3. 删除服务配置文件 ==="
if [ -f "$SERVICE_FILE" ]; then
    sudo rm "$SERVICE_FILE"
    echo "已删除 $SERVICE_FILE"
else
    echo "未找到 $SERVICE_FILE，可能已经卸载过了"
fi

echo "=== 4. 重新加载 systemd 配置 ==="
sudo systemctl daemon-reload

echo "====================================================="
echo " 服务已成功卸载！"
echo " 程序文件和配置文件未被删除，如需清理请手动删除。"
echo "====================================================="

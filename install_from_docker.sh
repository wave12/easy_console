#!/bin/bash
set -e

echo "🚀 开始通过 Docker 部署 EasyConsole..."

# 1. 检查 Docker
if ! command -v docker &> /dev/null; then
    echo "❌ 未检测到 Docker，请先安装 Docker。"
    exit 1
fi

# 2. 配置向导 (如果当前目录没有 config.json)
if [ ! -f config.json ]; then
    echo "=============================================="
    echo "首次运行，请选择您的数据上报方式："
    echo "  1) HTTP (向接口发送 POST 请求)"
    echo "  2) MQTT (连接 Broker 发送)"
    echo "=============================================="
    read -p "请输入数字选择 [1 或 2]: " PROTOCOL_CHOICE

    if [ "$PROTOCOL_CHOICE" == "1" ]; then
        # HTTP 配置流程
        echo "--- 配置 HTTP 上报 ---"
        read -p "请输入接收数据的 HTTP 接口地址 (例如 http://api.yourdomain.com/report): " HTTP_URL
        read -p "请输入接口验证 Token (如果没有直接回车): " HTTP_TOKEN
        
        cat <<EOF > config.json
{
  "report_protocol": "http",
  "http_url": "$HTTP_URL",
  "http_token": "$HTTP_TOKEN"
}
EOF

    elif [ "$PROTOCOL_CHOICE" == "2" ]; then
        # MQTT 配置流程
        echo "--- 配置 MQTT 上报 ---"
        read -p "请输入 MQTT 服务器地址 (例如 tcp://1.2.3.4:1883): " MQTT_HOST
        read -p "请输入 MQTT 用户名: " MQTT_USER
        read -sp "请输入 MQTT 密码: " MQTT_PASS
        echo "" # 换行
        
        cat <<EOF > config.json
{
  "report_protocol": "mqtt",
  "mqtt_server": "$MQTT_HOST",
  "mqtt_username": "$MQTT_USER",
  "mqtt_password": "$MQTT_PASS"
}
EOF

    else
        echo "❌ 输入错误，退出安装。"
        exit 1
    fi
    
    echo "✅ 配置文件 config.json 已生成！"
else
    echo "✅ 检测到已有的 config.json，跳过配置步骤。"
fi

# 检查如果本地没有这个镜像，就先构建
if [[ "$(docker images -q wave12/easy_console:latest 2> /dev/null)" == "" ]]; then
    echo "📦 本地未找到镜像，正在根据当前目录的 Dockerfile 构建（这可能需要几分钟）..."
    docker build -t wave12/easy_console:latest .
fi

if [ "$(docker ps -aq -f name=^/easy_console$)" ]; then
    echo "🧹 检测到旧容器，正在清理..."
    docker rm -f easy_console
fi

echo "📦 正在启动容器..."
docker run -d --name easy_console --network host \
  -v $(pwd)/config.json:/app/config.json \
  wave12/easy_console:latest


echo "✅ 启动成功！请使用 docker logs -f easy_console 查看运行状态。"
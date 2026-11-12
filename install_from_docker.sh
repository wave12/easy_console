#!/bin/bash
set -e

echo "🚀 开始通过 Docker 部署 easy_console..."

# 1. 检查 Docker
if ! command -v docker &> /dev/null; then
    echo "❌ 未检测到 Docker，请先安装 Docker。"
    exit 1
fi

# 2. 配置向导 (如果当前目录没有 config.json)
if [ ! -f config.json ]; then
    echo "⚠️ 未检测到 config.json，正在为您创建默认配置..."
    cp config.example.json config.json
    echo "=========================================================="
    echo "👉 请先编辑当前目录下的 config.json 文件！"
    echo "👉 填入您的 MQTT 服务器、账号密码、需要监控的磁盘和进程。"
    echo "👉 修改保存后，请重新运行本脚本： ./install_from_docker.sh"
    echo "=========================================================="
    exit 0  # 退出脚本，让用户去改文件
fi

# 3. 检查镜像并构建
if [[ "$(docker images -q wave12/easy_console:latest 2> /dev/null)" == "" ]]; then
    echo "📦 本地未找到镜像，正在根据当前目录的 Dockerfile 构建（这可能需要几分钟）..."
    docker build -t wave12/easy_console:latest .
fi

# 4. 清理旧容器
if [ "$(docker ps -aq -f name=^/easy_console$)" ]; then
    echo "🧹 检测到旧容器，正在清理..."
    docker rm -f easy_console > /dev/null
fi

# 5. 启动容器
echo "📦 正在启动容器..."
docker run -d --name easy_console --network host \
  --pid=host \
  -v /proc:/host/proc:ro \
  -v $(pwd)/config.json:/app/config.json \
  wave12/easy_console:latest

echo "✅ 启动成功！"
echo "=========================================================="
echo "💡 提示：如果您以后需要修改配置，请直接编辑当前目录的 config.json，"
echo "   然后执行 'docker restart easy_console' 即可生效。"
echo "   查看运行日志请执行： 'docker logs -f easy_console'"
echo "=========================================================="

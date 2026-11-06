#!/bin/bash
set -e # 遇到错误立即停止

# 获取脚本所在目录的绝对路径，保证不管在哪执行，都能找到相对路径
PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
cd "$PROJECT_ROOT"

echo "=== 1. 安装基础依赖 ==="
sudo apt update
sudo apt install -y libssl-dev cmake build-essential git

echo "=== 2. 编译并安装 Paho MQTT C 库 ==="
cd "${PROJECT_ROOT}/third_party/paho.mqtt.c"
rm -rf build
mkdir -p build && cd build
cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_SHARED=ON -DPAHO_BUILD_STATIC=ON
make -j$(nproc)
sudo make install

echo "=== 3. 编译并安装 Paho MQTT C++ 库 ==="
cd "${PROJECT_ROOT}/third_party/paho.mqtt.cpp"
rm -rf build
mkdir -p build && cd build
cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_SHARED=ON -DPAHO_BUILD_STATIC=ON
make -j$(nproc)
sudo make install

echo "=== 4. 刷新系统配置 ==="
sudo ldconfig
if [ -d /usr/local/lib ]; then
    sudo ln -sf /usr/local/lib/libpaho-mqtt* /usr/lib/ 2>/dev/null || true
fi

echo "=== 5. 编译你的 EasyConsole 项目 ==="
cd "$PROJECT_ROOT" 
rm -rf build
mkdir -p build && cd build
cmake .. 
make -j$(nproc)

echo "========================================="
echo " 🎉 编译成功！可执行文件在 build/easy_console"
echo "========================================="

#!/bin/bash
# =====================================================
# Paho MQTT C 与 C++ 库一键安装脚本
# =====================================================

# 遇到任何错误立即停止执行
set -e

echo "=== 1. 安装基础依赖 ==="
sudo apt update
sudo apt install -y libssl-dev cmake build-essential git

echo "=== 2. 编译并安装 Paho MQTT C 库 ==="
cd ~
# 如果之前克隆过，先删掉，保证是最干净的拉取
rm -rf paho.mqtt.c
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
mkdir -p build && cd build
# PAHO_WITH_SSL=ON 支持加密，PAHO_BUILD_SHARED/STATIC 同时生成动静态库
cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_SHARED=ON -DPAHO_BUILD_STATIC=ON
make -j$(nproc)
sudo make install

echo "=== 3. 编译并安装 Paho MQTT C++ 库 ==="
cd ~
rm -rf paho.mqtt.cpp
git clone https://github.com/eclipse/paho.mqtt.cpp.git
cd paho.mqtt.cpp
mkdir -p build && cd build
cmake .. -DPAHO_BUILD_SHARED=ON -DPAHO_BUILD_STATIC=ON
make -j$(nproc)
sudo make install

echo "=== 4. 刷新系统动态链接库缓存 ==="
sudo ldconfig

echo "====================================================="
echo " Paho MQTT 库安装完成！"
echo " 你可以运行 'ls /usr/local/lib/libpaho*' 来验证"
echo "====================================================="

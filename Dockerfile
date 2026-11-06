FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# 1. 换成阿里云源，极速安装基础依赖
RUN sed -i 's@archive.ubuntu.com@mirrors.aliyun.com@g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    build-essential cmake libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. 把提前下载好的源码拷贝进容器
COPY third_party/paho.mqtt.c /tmp/paho.mqtt.c
COPY third_party/paho.mqtt.cpp /tmp/paho.mqtt.cpp

# 3. 编译并安装 Paho C
RUN cd /tmp/paho.mqtt.c && mkdir build && cd build && \
    cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SHARED=ON && \
    make -j$(nproc) && make install && ldconfig

# 4. 编译并安装 Paho C++
RUN cd /tmp/paho.mqtt.cpp && mkdir build && cd build && \
    cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SHARED=ON && \
    make -j$(nproc) && make install && ldconfig

# 5. 拷贝项目代码并编译
WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && \
    rm -rf * && \
    cmake .. && make -j$(nproc)

CMD ["./build/easy_console"]
# ==========================================
# 第一阶段：编译阶段 (Builder) - 也就是“建筑工地”
# ==========================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. 换阿里云源，安装编译工具
RUN sed -i 's@archive.ubuntu.com@mirrors.aliyun.com@g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    build-essential cmake libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. 拷贝并编译 Paho C 库
COPY third_party/paho.mqtt.c /tmp/paho.mqtt.c
RUN cd /tmp/paho.mqtt.c && mkdir -p build && cd build && \
    cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SHARED=ON && \
    make -j$(nproc) && make install && ldconfig

# 3. 拷贝并编译 Paho C++ 库
COPY third_party/paho.mqtt.cpp /tmp/paho.mqtt.cpp
RUN cd /tmp/paho.mqtt.cpp && mkdir -p build && cd build && \
    cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SHARED=ON && \
    make -j$(nproc) && make install && ldconfig

# 4. 拷贝你的代码并编译
WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && \
    rm -rf * && \
    cmake .. && make -j$(nproc)

# ==========================================
# 第二阶段：运行阶段 (Runner) - 也就是“最终交付的车”
# ==========================================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 运行阶段只需要极少的基础库（甚至不需要 cmake、gcc、git）
# 只需要 libssl3 保证 HTTPS 和 MQTT 加密能用，ca-certificates 保证证书校验
RUN sed -i 's@archive.ubuntu.com@mirrors.aliyun.com@g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    libssl3 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 从第一阶段（builder）把编译好的程序、头文件和动态库拷贝过来
WORKDIR /app
COPY --from=builder /app/build/easy_console /app/easy_console
COPY --from=builder /usr/local/lib/libpaho* /usr/local/lib/
COPY --from=builder /usr/local/include/mqtt /usr/local/include/mqtt

# 刷新动态链接库缓存，确保能找到刚才拷过来的 Paho 库
RUN ldconfig

# 启动你的程序
CMD ["./easy_console"]

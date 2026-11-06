# easy_console

开源服务器监控与远程管控控制台 (Open-source server monitoring and remote management console)

## 1. 程序结构

```text
easy_console/
├── CMakeLists.txt         # 构建配置文件
├── README.md
├── config.json            # 配置文件
├── env_setup.sh           # 环境安装脚本
├── install_service.sh     # systemd 服务安装脚本
├── uninstall_service.sh   # systemd 服务卸载脚本
├── include/
│   ├── models/
│   │   └── MetricData.h   # 数据结构定义
│   ├── config/
│   │   └── GlobalConfig.h # 全局配置类
│   ├── collectors/
│   │   ├── ICollector.h   # 采集器基类接口
│   │   └── CollectorFactory.h # 工厂类，用于跨平台实例化
│   └── sender/
│       ├── ISender.h      # 上报器基类接口
│       └── HttpSender.h   # HTTP 上报器接口
├── src/
│   ├── main.cpp           # 主线程调度
│   ├── collectors/
│   │   ├── LinuxCollector.cpp # Linux 平台实现
│   │   └── WindowsCollector.cpp # Windows 平台实现
│   └── sender/
│       ├── HttpSender.cpp
│       └── MqttSender.cpp # 预留
├── third_party/
│   ├── nlohmann/json.hpp  # 引入 JSON 库（单头文件）
│   └── httplib.h          # 引入 HTTP 库（单头文件）
└── test/
    └── httpServer.py      # 测试 HTTP 服务器接口
```

## 2. 环境安装

执行 env_setup.sh 脚本安装依赖库：
```bash
./env_setup.sh
```

## 3. 构建命令

进入项目根目录，执行以下命令：
```bash
mkdir build
cd build
cmake ..
make
```

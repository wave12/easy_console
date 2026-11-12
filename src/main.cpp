#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <csignal>
#include <atomic>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>
#include "collectors/ICollector.h"
#include "models/MetricData.h"
#include "config/GlobalConfig.h"

#include "sender/ISender.h"
#include "sender/HttpSender.h"
#include "sender/MqttSender.h"

using json = nlohmann::json;

// 全局运行标志位，用于控制优雅退出
std::atomic<bool> g_running(true);

// 信号处理函数（响应 Ctrl+C 或 kill 信号）
void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down gracefully..." << std::endl;
    g_running = false;
}

// 模拟读取配置文件
// 在 main.cpp 的 loadConfig 函数中添加：
void loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Config file not found, using default." << std::endl;
        return;
    }

    try {
        json config;
        file >> config;
        
        auto& gc = GlobalConfig::getInstance();

        if (config.contains("collect_interval")) {
            gc.collect_interval = config["collect_interval"].get<int>();
            if (gc.collect_interval <= 0) gc.collect_interval = 30; // 防止填错变负数
        }

        if (config.contains("agent_token")) gc.agent_token = config["agent_token"].get<std::string>();
        if (config.contains("node_name")) gc.node_name = config["node_name"].get<std::string>();
        if (config.contains("decimal_places")) gc.decimal_places.store(config["decimal_places"].get<int>());
        if (config.contains("sender_type")) gc.sender_type = config["sender_type"].get<std::string>();
        if (config.contains("server_url")) gc.server_url = config["server_url"].get<std::string>();
        
        if (config.contains("mqtt_broker")) gc.mqtt_broker = config["mqtt_broker"].get<std::string>();
        if (config.contains("mqtt_username")) gc.mqtt_username = config["mqtt_username"].get<std::string>();
        if (config.contains("mqtt_password")) gc.mqtt_password = config["mqtt_password"].get<std::string>();
        if (config.contains("mqtt_topic")) gc.mqtt_topic = config["mqtt_topic"].get<std::string>();
        if (config.contains("mqtt_client_prefix")) gc.mqtt_client_prefix = config["mqtt_client_prefix"].get<std::string>();
      
        // 2. 解析需要监控的磁盘路径
        if (config.contains("disk_mount_points") && config["disk_mount_points"].is_array()) {
            gc.disk_mount_points.clear();
            for (const auto& path : config["disk_mount_points"]) {
                gc.disk_mount_points.push_back(path.get<std::string>());
            }
            std::cout << "Loaded " << gc.disk_mount_points.size() << " disk paths to monitor." << std::endl;
        }

        if (config.contains("monitor_processes") && config["monitor_processes"].is_array()) {
            gc.monitor_processes.clear();
            for (const auto& proc : config["monitor_processes"]) {
                gc.monitor_processes.push_back(proc.get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse config: " << e.what() << std::endl;
    }
}

// 获取当前时间，格式：YYYY-MM-DD HH:MM:SS.mmm
std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // 获取毫秒部分 (0-999)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::tm tm_now;
    // 跨平台的本地时间转换
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif

    std::ostringstream oss;
    // 格式化年月日时分秒
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    // 加上毫秒，setfill('0') 和 setw(3) 保证毫秒是3位数
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

int main() {
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 加载配置config.json 文件
    loadConfig("config.json");
    auto& gc = GlobalConfig::getInstance();

    auto collector = CollectorFactory::create();
    SystemMetrics metrics; 

    // 预热：在进入循环前先采集一次 CPU，打底
    // 因为第一次算不出差值，我们要保证第一次上报的数据是真实的
    CpuMetric dummy;
    collector->collectCPU(dummy); 

    int collect_interval_sec = gc.collect_interval; // 采集间隔 
    std::cout << "Monitor Agent started. Collecting every " << collect_interval_sec << " seconds." << std::endl;

    // 根据配置决定实例化哪个发送器
    std::unique_ptr<ISender> sender;
    if (gc.sender_type == "mqtt") {
        sender = std::make_unique<MqttSender>(gc.mqtt_broker, gc.mqtt_client_prefix, gc.mqtt_topic, gc.mqtt_username, gc.mqtt_password);
    } else {
        sender = std::make_unique<HttpSender>(gc.server_url, gc.agent_token);
    }

    while (g_running) {
        auto start_time = std::chrono::steady_clock::now();

        // 1. 串行或并发采集（对于本地 /proc 读取，串行其实极快，几毫秒而已）
        // 如果你特别在意这十几毫秒的耗时，可以用 std::thread 并发，这里为了稳定用串行演示
        collector->collectCPU(metrics.cpu);
        collector->collectMemory(metrics.mem);
        collector->collectDisks(metrics.disks);
        collector->collectProcesses(metrics.top_processes);

        // 2. 转换为 JSON
        json j = metrics;
        j["time_str"] = getCurrentTimeString(); 
        j["timestamp"] = std::time(nullptr);
        j["node_name"] = gc.node_name;
        j["collect_interval"] = gc.collect_interval;
        
        // 生成无格式化的紧凑 JSON 字符串（减小网络传输体积）
        std::string payload = j.dump(); 
        
        // --- 在这里调用你的发送逻辑 ---
        std::cout << "[" << getCurrentTimeString() << "] Payload: " << payload << std::endl;              

        // 发送数据
        sender->send(payload);

        // 3. 精确控制采集间隔（防止时间漂移）
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        
        int sleep_time = collect_interval_sec - elapsed;
        if (sleep_time > 0) {
            // 将休眠时间分片，每 100 毫秒检查一次 g_running 标志
            // 这样当你按 Ctrl+C 时，程序能瞬间响应，不用等 5 秒休眠结束
            for (int i = 0; i < sleep_time * 10 && g_running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    std::cout << "Monitor Agent stopped." << std::endl;
    return 0;
}
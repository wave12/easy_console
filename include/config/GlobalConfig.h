#pragma once
#include <atomic>

class GlobalConfig {
public:
    // 获取单例实例
    static GlobalConfig& getInstance() {
        static GlobalConfig instance;
        return instance;
    }

    std::string agent_token = "";
    std::string node_name = "";

    int collect_interval = 30;

    // 小数保留位数，默认 2 位
    std::atomic<int> decimal_places{2};
    std::string sender_type{"http"};

    std::string server_url{"http://127.0.0.1:8080"};

    // MQTT 配置
    std::string mqtt_broker{"tcp://127.0.0.1:1883"};
    std::string mqtt_username{""};  
    std::string mqtt_password{""};  
    std::string mqtt_topic{"monitor/agent/metrics"};
    std::string mqtt_client_prefix{"nl_monitor_"};

    // 需要监控的磁盘挂载点列表
    // 如果列表为空，我们可以选择默认监控 "/"
    std::vector<std::string> disk_mount_points{"/"};

    // 需要监控的进程名列表
    std::vector<std::string> monitor_processes{"nginx"};

private:
    GlobalConfig() = default;
    GlobalConfig(const GlobalConfig&) = delete;
    GlobalConfig& operator=(const GlobalConfig&) = delete;
};

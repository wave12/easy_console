#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "config/GlobalConfig.h"

using json = nlohmann::json;

// 辅助函数：按全局配置保留小数位数
inline json formatDouble(double value) {
    int places = GlobalConfig::getInstance().decimal_places.load();
    if (places < 0) return value; // 负数表示不限制精度

    // 四舍五入
    double factor = std::pow(10.0, places);
    double rounded = std::round(value * factor) / factor;

    // 智能判断：如果是个整数（比如 100.0），直接返回整数类型
    if (rounded == std::floor(rounded)) {
        return static_cast<long long>(rounded);
    }
    
    // 否则返回浮点数
    return rounded;
}

// ==========================================
// 1. 定义数据结构（纯数据，不包含成员函数）
// ==========================================

struct CpuMetric {
    double usage_percent;
    int core_count;
};

struct MemMetric {
    uint64_t total_mb;
    uint64_t used_mb;
};

struct DiskMetric {
    std::string mount_point;
    double usage_percent;
};

struct ProcessMetric {
    int pid;
    std::string name;
    double cpu_usage;
    double mem_usage;
    bool is_running;
};

struct SystemMetrics {
    CpuMetric cpu;
    MemMetric mem;
    std::vector<DiskMetric> disks;
    std::vector<ProcessMetric> top_processes;
};

// ==========================================
// 2. 定义非成员的 to_json 函数
// (nlohmann/json 会自动在全局查找这些函数)
// ==========================================

inline void to_json(json& j, const CpuMetric& p) {
    j = json{{"usage", formatDouble(p.usage_percent)}, {"cores", p.core_count}};
}

inline void to_json(json& j, const MemMetric& p) {
    j = json{{"total_mb", p.total_mb}, {"used_mb", p.used_mb}};
}

inline void to_json(json& j, const DiskMetric& p) {
    j = json{{"mount_point", p.mount_point}, {"usage", formatDouble(p.usage_percent)}};
}

inline void to_json(json& j, const ProcessMetric& p) {
    j = json{
        {"pid", p.pid}, 
        {"name", p.name}, 
        {"cpu_usage", formatDouble(p.cpu_usage)}, 
        {"mem_usage", formatDouble(p.mem_usage)},
        {"is_running", p.is_running}
    };
}

inline void to_json(json& j, const SystemMetrics& p) {
    j = json{
        {"cpu", p.cpu},
        {"mem", p.mem},
        {"disks", p.disks},
        {"processes", p.top_processes}
    };
}


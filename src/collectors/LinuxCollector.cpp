#include "collectors/ICollector.h"

#ifdef __linux__
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <thread>
#include <cstring>
#include <map>
#include <algorithm>
#include <vector>
#include <iostream>

class LinuxCollector : public ICollector {
private:
    // 用于计算 CPU 使用率的静态变量（保存上一次的值）
    unsigned long long last_total_user = 0, last_total_user_low = 0;
    unsigned long long last_total_sys = 0, last_total_idle = 0;

    // 辅助函数：解析 /proc/[pid]/stat 文件，提取进程名和内存信息
    ProcessMetric getProcessMetric(int pid) {
        ProcessMetric p;
        p.pid = pid;
        p.cpu_usage = 0.0;
        p.mem_usage = 0.0;
        p.name = "unknown";

        std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
        std::ifstream stat_file(stat_path);
        if (!stat_file.is_open()) return p;

        std::string line;
        std::getline(stat_file, line);

        // 解析 stat 文件格式：pid (comm) state ppid ...
        size_t left_paren = line.find('(');
        size_t right_paren = line.rfind(')');
        if (left_paren != std::string::npos && right_paren != std::string::npos) {
            p.name = line.substr(left_paren + 1, right_paren - left_paren - 1);
        }

        // 获取进程占用的物理内存 (RSS)
        std::string statm_path = "/proc/" + std::to_string(pid) + "/statm";
        std::ifstream statm_file(statm_path);
        if (statm_file.is_open()) {
            long size, resident;
            statm_file >> size >> resident;
            // resident 单位是页 (page)，通常 1 页 = 4KB
            p.mem_usage = (resident * 4096.0) / (1024.0 * 1024.0); // 转换为 MB
        }
        
        // 注：计算单进程的准确 CPU% 比较复杂（需要保存每个进程上次的 utime+stime），
        // 此处为了性能，简化处理，后续可用 pid 维护一个 map 来计算差值。
        return p;
    }

public:
    void collectCPU(CpuMetric& out) override {
        std::ifstream file("/proc/stat");
        std::string line;
        if (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string cpu;
            unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
            ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

            unsigned long long total_user = user + nice;
            unsigned long long total_sys = system + irq + softirq;
            unsigned long long total_idle = idle + iowait;

            unsigned long long diff_user = total_user - last_total_user;
            unsigned long long diff_sys = total_sys - last_total_sys;
            unsigned long long diff_idle = total_idle - last_total_idle;

            unsigned long long total_diff = diff_user + diff_sys + diff_idle;

            if (total_diff > 0 && last_total_user != 0) {
                out.usage_percent = 100.0 * (diff_user + diff_sys) / total_diff;
            } else {
                out.usage_percent = 0.0; // 第一次采集算不出差值
            }

            last_total_user = total_user;
            last_total_sys = total_sys;
            last_total_idle = total_idle;
        }
        out.core_count = std::thread::hardware_concurrency();
    }

    void collectMemory(MemMetric& out) override {
        std::ifstream file("/proc/meminfo");
        std::string line;
        uint64_t mem_total = 0, mem_available = 0;

        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string key;
            uint64_t value;
            ss >> key >> value;
            
            if (key == "MemTotal:") {
                mem_total = value; // 单位 kB
            } else if (key == "MemAvailable:") {
                mem_available = value;
            }
            
            // 如果两个都找到了，提前退出循环
            if (mem_total > 0 && mem_available > 0) break;
        }

        out.total_mb = mem_total / 1024;
        out.used_mb = (mem_total - mem_available) / 1024;
    }

    void collectDisks(std::vector<DiskMetric>& out) override {
        out.clear();
        
        // 从全局配置获取需要监控的路径
        const auto& paths = GlobalConfig::getInstance().disk_mount_points;
    
        for (const auto& path : paths) {
            struct statvfs vfs;
            // 直接对指定路径进行 statvfs 查询
            if (statvfs(path.c_str(), &vfs) == 0) {
                DiskMetric disk;
                disk.mount_point = path;
                
                uint64_t total_bytes = vfs.f_blocks * vfs.f_frsize;
                uint64_t free_bytes = vfs.f_bfree * vfs.f_frsize;
                
                if (total_bytes > 0) {
                    disk.usage_percent = 100.0 * (total_bytes - free_bytes) / total_bytes;
                    out.push_back(disk);
                }
            } else {
                // 如果路径不存在或没有权限，可以选择跳过，或者记录一个特殊值
                // 这里选择忽略
            }
        }
    }

    void collectProcesses(std::vector<ProcessMetric>& out) override {
        out.clear();
        
        const auto& target_names = GlobalConfig::getInstance().monitor_processes;
        
        // 1. 初始化结果集：先假设所有配置的进程都没运行
        std::map<std::string, ProcessMetric> result_map;
        for (const auto& name : target_names) {
            ProcessMetric p;
            p.name = name;
            p.pid = -1; // 未运行的进程 PID 设为 -1
            p.cpu_usage = 0.0;
            p.mem_usage = 0.0;
            p.is_running = false;
            result_map[name] = p;
        }
    
        if (target_names.empty()) return; // 没配就不扫
    
        // 2. 遍历 /proc 寻找匹配的进程
        DIR* dir = opendir("/proc");
        if (!dir) return;
    
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                int pid = std::stoi(entry->d_name);
                
                std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
                std::ifstream stat_file(stat_path);
                if (!stat_file.is_open()) continue;
    
                std::string line;
                std::getline(stat_file, line);
                
                // 提取进程名 (在括号内)
                size_t left_paren = line.find('(');
                size_t right_paren = line.rfind(')');
                if (left_paren != std::string::npos && right_paren != std::string::npos) {
                    std::string proc_name = line.substr(left_paren + 1, right_paren - left_paren - 1);
                    
                    // 如果这个进程名在我们配置的监控列表里
                    if (result_map.find(proc_name) != result_map.end()) {
                        // 标记为运行中
                        result_map[proc_name].is_running = true;
                        
                        // 如果同名进程有多个，记录第一个找到的 PID 即可，或者记录 -2 表示有多个
                        if (result_map[proc_name].pid == -1) {
                            result_map[proc_name].pid = pid;
                        }
    
                        // 获取物理内存 (statm)
                        std::string statm_path = "/proc/" + std::to_string(pid) + "/statm";
                        std::ifstream statm_file(statm_path);
                        if (statm_file.is_open()) {
                            long size, resident;
                            statm_file >> size >> resident;
                            // 累加内存占用 (MB)
                            result_map[proc_name].mem_usage += (resident * 4096.0) / (1024.0 * 1024.0);
                        }
    
                        // 获取 CPU 时间 (暂不细化单进程 CPU 百分比，前面讨论过开销大，这里仅保留占位)
                        // 如果一定要算，需要将 utime+stime 记录并在全局维护上一次的值
                    }
                }
            }
        }
        closedir(dir);
    
        // 3. 将结果转换为 vector 输出
        for (const auto& pair : result_map) {
            out.push_back(pair.second);
        }
    }
};

// 工厂方法实现
std::unique_ptr<ICollector> CollectorFactory::create() {
    return std::make_unique<LinuxCollector>();
}

#endif // __linux__
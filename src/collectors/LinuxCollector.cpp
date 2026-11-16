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
#include <unistd.h>
#include <stdexcept> // 用于异常处理

class LinuxCollector : public ICollector {
private:
    unsigned long long last_total_user = 0, last_total_user_low = 0;
    unsigned long long last_total_sys = 0, last_total_idle = 0;

    std::string getProcPath() {
        if (access("/host/proc", F_OK) == 0 && access("/.dockerenv", F_OK) == 0) {
            return "/host/proc";
        }
        return "/proc";
    }

public:
    void collectCPU(CpuMetric& out) override {
        // 👇 修复：使用动态获取的路径
        std::string stat_path = getProcPath() + "/stat";
        std::ifstream file(stat_path);
        if (!file.is_open()) return;

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
                out.usage_percent = 0.0;
            }

            last_total_user = total_user;
            last_total_sys = total_sys;
            last_total_idle = total_idle;
        }
        out.core_count = std::thread::hardware_concurrency();
    }

    void collectMemory(MemMetric& out) override {
        // 👇 修复：使用动态获取的路径
        std::string meminfo_path = getProcPath() + "/meminfo";
        std::ifstream file(meminfo_path);
        if (!file.is_open()) return;

        std::string line;
        uint64_t mem_total = 0, mem_available = 0;

        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string key;
            uint64_t value;
            ss >> key >> value;
            
            if (key == "MemTotal:") {
                mem_total = value;
            } else if (key == "MemAvailable:") {
                mem_available = value;
            }
            if (mem_total > 0 && mem_available > 0) break;
        }

        out.total_mb = mem_total / 1024;
        out.used_mb = (mem_total - mem_available) / 1024;
    }

    void collectDisks(std::vector<DiskMetric>& out) override {
        out.clear();
        const auto& paths = GlobalConfig::getInstance().disk_mount_points;
    
        for (const auto& path : paths) {
            struct statvfs vfs;
            if (statvfs(path.c_str(), &vfs) == 0) {
                DiskMetric disk;
                disk.mount_point = path;
                uint64_t total_bytes = vfs.f_blocks * vfs.f_frsize;
                uint64_t free_bytes = vfs.f_bfree * vfs.f_frsize;
                if (total_bytes > 0) {
                    disk.usage_percent = 100.0 * (total_bytes - free_bytes) / total_bytes;
                    out.push_back(disk);
                }
            }
        }
    }

    void collectProcesses(std::vector<ProcessMetric>& out) override {
        out.clear();
        const auto& target_names = GlobalConfig::getInstance().monitor_processes;
        if (target_names.empty()) return;

        std::map<std::string, ProcessMetric> result_map;
        for (const auto& name : target_names) {
            ProcessMetric p;
            p.name = name;
            p.pid = -1;
            p.cpu_usage = 0.0;
            p.mem_usage = 0.0;
            p.is_running = false;
            result_map[name] = p;
        }
    
        std::string proc_base = getProcPath();
        std::string proc_dir_path = proc_base + "/";

        DIR* dir = opendir(proc_dir_path.c_str());
        if (!dir) return;
    
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // 👇 修复：加上了防空指针和防崩溃保护
            if (entry->d_type == DT_DIR && entry->d_name[0] != '\0' && isdigit(entry->d_name[0])) {
                int pid = 0;
                try {
                    pid = std::stoi(entry->d_name); // 防止意外抛出异常
                } catch (...) {
                    continue;
                }
                
                std::string stat_path = proc_base + "/" + std::to_string(pid) + "/stat";
                std::ifstream stat_file(stat_path);
                if (!stat_file.is_open()) continue;
    
                std::string line;
                if (std::getline(stat_file, line)) {
                    size_t left_paren = line.find('(');
                    size_t right_paren = line.rfind(')');
                    if (left_paren != std::string::npos && right_paren != std::string::npos) {
                        std::string proc_name = line.substr(left_paren + 1, right_paren - left_paren - 1);
                        
                        if (result_map.find(proc_name) != result_map.end()) {
                            result_map[proc_name].is_running = true;
                            
                            if (result_map[proc_name].pid == -1) {
                                result_map[proc_name].pid = pid;
                            }
        
                            std::string statm_path = proc_base + "/" + std::to_string(pid) + "/statm";
                            std::ifstream statm_file(statm_path);
                            if (statm_file.is_open()) {
                                long size, resident;
                                if (statm_file >> size >> resident) {
                                    result_map[proc_name].mem_usage += (resident * 4096.0) / (1024.0 * 1024.0);
                                }
                            }
                        }
                    }
                }
            }
        }
        closedir(dir);
    
        for (const auto& pair : result_map) {
            out.push_back(pair.second);
        }
    }
};

std::unique_ptr<ICollector> CollectorFactory::create() {
    return std::make_unique<LinuxCollector>();
}

#endif // __linux__
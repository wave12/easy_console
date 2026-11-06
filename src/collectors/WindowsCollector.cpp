#include "collectors/ICollector.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

class WindowsCollector : public ICollector {
public:
    void collectCPU(CpuMetric& out) override {
        // 使用 GetSystemTimes 获取内核时间、用户时间、空闲时间
        // 计算出使用率（需保存上次数据做差值）
        out.core_count = std::thread::hardware_concurrency();
    }
    
    void collectMemory(MemMetric& out) override {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        out.total_mb = memInfo.ullTotalPhys / (1024 * 1024);
        out.used_mb = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024);
    }

    // ... 省略 Disk 和 Process 的 Windows API 实现
};

std::unique_ptr<ICollector> CollectorFactory::create() {
    return std::make_unique<WindowsCollector>();
}
#endif
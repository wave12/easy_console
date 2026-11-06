#pragma once
#include <vector>
#include "models/MetricData.h"
#include <memory>

class ICollector {
public:
    virtual ~ICollector() = default;
    virtual void collectCPU(CpuMetric& out) = 0;
    virtual void collectMemory(MemMetric& out) = 0;
    virtual void collectDisks(std::vector<DiskMetric>& out) = 0;
    virtual void collectProcesses(std::vector<ProcessMetric>& out) = 0;
};

class CollectorFactory {
public:
    static std::unique_ptr<ICollector> create();
};
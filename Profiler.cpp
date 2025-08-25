#include "Profiler.h"
#include "Logger.h"
#include <iostream>
#include <iomanip>

std::unique_ptr<Profiler> Profiler::instance = nullptr;
std::once_flag Profiler::initFlag;

Profiler::Profiler() : enabled(true) {
}

Profiler& Profiler::GetInstance() {
    std::call_once(initFlag, []() {
        instance = std::unique_ptr<Profiler>(new Profiler());
    });
    return *instance;
}

void Profiler::StartTimer(const std::string& name) {
    if (!enabled) return;
    // This method is not used in RAII pattern, kept for compatibility
}

void Profiler::EndTimer(const std::string& name, double elapsedMs) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(timerMutex);
    
    auto& timer = timers[name];
    timer.totalTime += elapsedMs;
    timer.callCount++;
    
    if (elapsedMs < timer.minTime) {
        timer.minTime = elapsedMs;
    }
    if (elapsedMs > timer.maxTime) {
        timer.maxTime = elapsedMs;
    }
    
    timer.avgTime = timer.totalTime / timer.callCount;
}

void Profiler::PrintStatistics() const {
    if (!enabled || timers.empty()) return;
    
    std::lock_guard<std::mutex> lock(timerMutex);
    
    LOG_INFO("=== Performance Statistics ===");
    LOG_INFO("{:<20} {:>10} {:>10} {:>10} {:>10} {:>10}",
        "Timer", "Calls", "Total(ms)", "Avg(ms)", "Min(ms)", "Max(ms)");
    
    for (const auto& [name, data] : timers) {
        LOG_INFO("{:<20} {:>10} {:>10.3f} {:>10.3f} {:>10.3f} {:>10.3f}",
            name, data.callCount, data.totalTime, data.avgTime, data.minTime, data.maxTime);
    }
    LOG_INFO("===============================");
}

void Profiler::Reset() {
    std::lock_guard<std::mutex> lock(timerMutex);
    timers.clear();
}

Profiler::TimerData Profiler::GetTimerData(const std::string& name) const {
    std::lock_guard<std::mutex> lock(timerMutex);
    
    auto it = timers.find(name);
    if (it != timers.end()) {
        return it->second;
    }
    return TimerData{};
}

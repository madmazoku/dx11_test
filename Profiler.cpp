/**
 * @file Profiler.cpp
 * @brief Implementation of the Profiler class for performance monitoring and analysis
 * 
 * The Profiler provides comprehensive performance monitoring capabilities for the
 * particle simulation system. It tracks execution times for different subsystems,
 * maintains statistical data, and provides detailed performance reports for
 * optimization and debugging purposes.
 * 
 * Key features:
 * - High-precision timing using std::chrono for accurate measurements
 * - Thread-safe singleton pattern for global profiling access
 * - Statistical analysis including min/max/average execution times
 * - Hierarchical profiling with nested timing blocks
 * - Configurable profiling enable/disable for production builds
 * - Detailed performance reports with formatted output
 * - Memory-efficient storage of timing data
 * 
 * The profiling system is essential for identifying performance bottlenecks
 * and optimizing the particle simulation for real-time performance.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#include "Profiler.h"
#include "Logger.h"
#include <iostream>
#include <iomanip>

// Static member initialization for singleton pattern
std::unique_ptr<Profiler> Profiler::instance = nullptr;
std::once_flag Profiler::initFlag;

/**
 * @brief Private constructor for singleton Profiler instance
 * 
 * Initializes the profiler with default settings (profiling enabled).
 * Private constructor ensures singleton pattern enforcement.
 */
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

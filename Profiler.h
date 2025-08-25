#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <limits>

class ProfilerTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::string name;
    
public:
    ProfilerTimer(const std::string& timerName);
    ~ProfilerTimer();
};

class Profiler {
private:
    struct TimerData {
        double totalTime = 0.0;
        double minTime = std::numeric_limits<double>::max();
        double maxTime = 0.0;
        double avgTime = 0.0;
        uint64_t callCount = 0;
    };
    
    static std::unique_ptr<Profiler> instance;
    static std::once_flag initFlag;
    
    std::unordered_map<std::string, TimerData> timers;
    std::mutex timerMutex;
    bool enabled;

    Profiler();

public:
    static Profiler& GetInstance();
    
    void SetEnabled(bool enable) { enabled = enable; }
    bool IsEnabled() const { return enabled; }
    
    void StartTimer(const std::string& name);
    void EndTimer(const std::string& name, double elapsedMs);
    
    void PrintStatistics() const;
    void Reset();
    
    TimerData GetTimerData(const std::string& name) const;
};

// RAII Timer for automatic profiling
class ScopedTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::string timerName;
    
public:
    ScopedTimer(const std::string& name) : timerName(name) {
        if (Profiler::GetInstance().IsEnabled()) {
            startTime = std::chrono::high_resolution_clock::now();
        }
    }
    
    ~ScopedTimer() {
        if (Profiler::GetInstance().IsEnabled()) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(endTime - startTime);
            Profiler::GetInstance().EndTimer(timerName, duration.count());
        }
    }
};

// Convenience macro for profiling
#define PROFILE_SCOPE(name) ScopedTimer _timer(name)
#define PROFILE_FUNCTION() ScopedTimer _timer(__FUNCTION__)

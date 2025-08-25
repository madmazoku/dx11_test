#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <limits>

/**
 * @file Profiler.h
 * @brief High-precision performance profiling system for optimization analysis
 * 
 * This file provides a comprehensive profiling system designed for performance
 * analysis and optimization of the particle simulation system. It offers both
 * manual timing control and automatic RAII-based profiling with statistical
 * analysis of timing data.
 * 
 * Key features:
 * - High-resolution timing using std::chrono::high_resolution_clock
 * - Thread-safe timer management for multi-threaded applications
 * - Statistical analysis (min, max, average, total time)
 * - RAII-based automatic profiling with ScopedTimer
 * - Convenient macros for easy integration
 * - Runtime enable/disable for production builds
 * - Named timer tracking for complex call hierarchies
 * 
 * The profiling system is essential for identifying performance bottlenecks
 * and validating optimization efforts in the particle simulation pipeline.
 */

/**
 * @class ProfilerTimer
 * @brief Legacy timer class for compatibility
 * 
 * Provides simple RAII timing functionality. This class is maintained
 * for backward compatibility but ScopedTimer is preferred for new code.
 */
class ProfilerTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime;   ///< Timer start timestamp
    std::string name;                                           ///< Timer identifier name
    
public:
    /**
     * @brief Construct and start timing
     * @param timerName Unique identifier for this timer
     */
    ProfilerTimer(const std::string& timerName);
    
    /**
     * @brief Destructor stops timing and records result
     */
    ~ProfilerTimer();
};

/**
 * @class Profiler
 * @brief Singleton performance profiling system with statistical analysis
 * 
 * Provides comprehensive performance profiling capabilities for the particle
 * simulation system. The profiler collects timing data from multiple sources
 * and maintains statistical information about performance characteristics.
 * 
 * The system is designed to be:
 * - Thread-safe: Multiple threads can profile simultaneously
 * - Low-overhead: Minimal impact on performance when enabled
 * - Statistical: Provides min/max/average timing analysis
 * - Flexible: Supports both manual and automatic timing
 * - Production-ready: Can be disabled entirely for release builds
 * 
 * Usage patterns:
 * 1. Use ScopedTimer for automatic RAII-based timing
 * 2. Use manual StartTimer/EndTimer for complex scenarios
 * 3. Call PrintStatistics() periodically to analyze performance
 * 4. Use Reset() to clear accumulated data between test runs
 */
class Profiler {
private:
    /**
     * @struct TimerData
     * @brief Statistical data collected for each named timer
     * 
     * Maintains comprehensive timing statistics for performance analysis,
     * including minimum, maximum, and average execution times along with
     * call frequency information.
     */
    struct TimerData {
        double totalTime = 0.0;                             ///< Cumulative time across all calls (milliseconds)
        double minTime = std::numeric_limits<double>::max(); ///< Minimum recorded time (milliseconds)
        double maxTime = 0.0;                               ///< Maximum recorded time (milliseconds)
        double avgTime = 0.0;                               ///< Average time per call (milliseconds)
        uint64_t callCount = 0;                             ///< Number of times this timer was used
    };
    
    // === Singleton Implementation ===
    static std::unique_ptr<Profiler> instance;     ///< Singleton instance pointer
    static std::once_flag initFlag;                ///< Thread-safe initialization flag
    
    // === Core Data Members ===
    std::unordered_map<std::string, TimerData> timers; ///< Storage for all timer statistics
    std::mutex timerMutex;                              ///< Mutex for thread-safe access
    bool enabled;                                       ///< Global enable/disable flag

    /**
     * @brief Private constructor for singleton pattern
     */
    Profiler();

public:
    // === Singleton Access ===
    
    /**
     * @brief Get the singleton profiler instance
     * 
     * Thread-safe access to the global profiler instance using std::once_flag
     * for guaranteed single initialization.
     * 
     * @return Reference to the singleton Profiler instance
     */
    static Profiler& GetInstance();
    
    // === Configuration ===
    
    /**
     * @brief Enable or disable profiling globally
     * 
     * When disabled, all profiling operations become no-ops, eliminating
     * performance overhead in production builds.
     * 
     * @param enable true to enable profiling, false to disable
     */
    void SetEnabled(bool enable) { enabled = enable; }
    
    /**
     * @brief Check if profiling is currently enabled
     * @return true if profiling is enabled, false otherwise
     */
    bool IsEnabled() const { return enabled; }
    
    // === Timer Management ===
    
    /**
     * @brief Start timing for a named timer
     * 
     * Begins timing for the specified timer name. Should be paired with
     * a corresponding EndTimer() call. For automatic timing, prefer
     * ScopedTimer instead.
     * 
     * @param name Unique identifier for the timer
     */
    void StartTimer(const std::string& name);
    
    /**
     * @brief End timing and record result
     * 
     * Completes timing for the specified timer and updates statistical
     * data with the elapsed time.
     * 
     * @param name Timer identifier (must match StartTimer call)
     * @param elapsedMs Elapsed time in milliseconds
     */
    void EndTimer(const std::string& name, double elapsedMs);
    
    // === Analysis and Reporting ===
    
    /**
     * @brief Print comprehensive timing statistics
     * 
     * Outputs formatted statistics for all recorded timers including
     * total time, call count, average, minimum, and maximum times.
     * Results are sent to the debug log.
     */
    void PrintStatistics() const;
    
    /**
     * @brief Reset all timing data
     * 
     * Clears all accumulated timing statistics, useful for starting
     * fresh measurement sessions or clearing startup overhead.
     */
    void Reset();
    
    /**
     * @brief Get timing data for a specific timer
     * 
     * Retrieves the statistical data for a named timer, useful for
     * programmatic analysis of performance metrics.
     * 
     * @param name Timer name to query
     * @return TimerData structure with statistical information
     */
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

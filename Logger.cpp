/**
 * @file Logger.cpp
 * @brief Implementation of the Logger class for comprehensive application logging
 * 
 * The Logger provides a thread-safe, singleton logging system with multiple severity
 * levels, formatted output, and flexible destination support. It handles all logging
 * needs for the particle simulation system including performance metrics, debugging
 * information, warnings, and error reporting.
 * 
 * Key features:
 * - Thread-safe singleton pattern with std::once_flag initialization
 * - Multiple log levels (Debug, Info, Warning, Error) with filtering
 * - High-precision timestamps with microsecond accuracy
 * - Formatted output with consistent message structure
 * - Console and file output support (extensible design)
 * - Macro-based logging interface for performance and convenience
 * 
 * The logging system is essential for debugging, performance analysis, and
 * runtime monitoring of the particle simulation system.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#include "Logger.h"
#include <iomanip>
#include <chrono>

// Static member initialization for singleton pattern
std::unique_ptr<Logger> Logger::instance = nullptr;
std::once_flag Logger::initFlag;

/**
 * @brief Private constructor for singleton Logger instance
 * 
 * Initializes the logger with default settings (Info level, console output enabled).
 * Private constructor ensures singleton pattern enforcement.
 */
Logger::Logger() : currentLevel(LogLevel::Info), consoleOutput(true) {
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger& Logger::GetInstance() {
    std::call_once(initFlag, []() {
        instance = std::unique_ptr<Logger>(new Logger());
    });
    return *instance;
}

void Logger::Initialize(const std::string& filename, LogLevel level, bool enableConsole) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    currentLevel = level;
    consoleOutput = enableConsole;
    
    if (logFile.is_open()) {
        logFile.close();
    }
    
    logFile.open(filename, std::ios::app);
    
    if (logFile.is_open()) {
        logFile << "\n=== New Session Started ===" << std::endl;
    }
}

std::string Logger::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_point = std::chrono::current_zone()->to_local(now);
    
    return std::format("{:%Y-%m-%d %H:%M:%S}", time_point);
}

std::string Logger::LogLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

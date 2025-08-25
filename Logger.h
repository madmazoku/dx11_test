#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <format>

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

class Logger {
private:
    static std::unique_ptr<Logger> instance;
    static std::once_flag initFlag;
    
    std::ofstream logFile;
    std::mutex logMutex;
    LogLevel currentLevel;
    bool consoleOutput;

    Logger();
    
    std::string GetTimestamp() const;
    std::string LogLevelToString(LogLevel level) const;

public:
    static Logger& GetInstance();
    
    void Initialize(const std::string& filename = "particle_simulation.log", 
                   LogLevel level = LogLevel::Info, 
                   bool enableConsole = true);
    
    void SetLogLevel(LogLevel level) { currentLevel = level; }
    void SetConsoleOutput(bool enable) { consoleOutput = enable; }
    
    template<typename... Args>
    void Log(LogLevel level, const std::string& format, Args&&... args) {
        if (level < currentLevel) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        std::string message;
        if constexpr (sizeof...(args) > 0) {
            message = std::format(format, std::forward<Args>(args)...);
        } else {
            message = format;
        }
        
        std::string timestamp = GetTimestamp();
        std::string levelStr = LogLevelToString(level);
        std::string fullMessage = std::format("[{}] [{}] {}", timestamp, levelStr, message);
        
        if (logFile.is_open()) {
            logFile << fullMessage << std::endl;
            logFile.flush();
        }
        
        if (consoleOutput) {
            if (level >= LogLevel::Error) {
                std::cerr << fullMessage << std::endl;
            } else {
                std::cout << fullMessage << std::endl;
            }
        }
    }
    
    template<typename... Args>
    void Debug(const std::string& format, Args&&... args) {
        Log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void Info(const std::string& format, Args&&... args) {
        Log(LogLevel::Info, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void Warning(const std::string& format, Args&&... args) {
        Log(LogLevel::Warning, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void Error(const std::string& format, Args&&... args) {
        Log(LogLevel::Error, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void Critical(const std::string& format, Args&&... args) {
        Log(LogLevel::Critical, format, std::forward<Args>(args)...);
    }
    
    ~Logger();
};

// Convenience macros
#define LOG_DEBUG(...) Logger::GetInstance().Debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::GetInstance().Info(__VA_ARGS__)
#define LOG_WARNING(...) Logger::GetInstance().Warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::GetInstance().Error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::GetInstance().Critical(__VA_ARGS__)

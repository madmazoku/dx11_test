#include "Logger.h"
#include <iomanip>
#include <chrono>

std::unique_ptr<Logger> Logger::instance = nullptr;
std::once_flag Logger::initFlag;

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

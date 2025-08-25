#include "Logger.h"
#include <iomanip>

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
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
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

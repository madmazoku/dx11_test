#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <format>

/**
 * @file Logger.h
 * @brief Thread-safe logging system with configurable output and formatting
 * 
 * This file provides a comprehensive logging system designed for multi-threaded
 * applications. It supports multiple log levels, both file and console output,
 * thread-safe operation, and modern C++20 formatting capabilities.
 * 
 * Key features:
 * - Thread-safe logging with mutex protection
 * - Multiple severity levels (Debug, Info, Warning, Error, Critical)
 * - Dual output: file and console with independent control
 * - Modern C++20 std::format support for type-safe formatting
 * - Singleton pattern for global access
 * - Automatic timestamping of all log messages
 * - Configurable log level filtering
 * - Convenient macros for easy usage
 * 
 * The logger is essential for debugging, monitoring, and troubleshooting
 * the particle simulation system during development and runtime.
 */

/**
 * @enum LogLevel
 * @brief Severity levels for log messages
 * 
 * Defines the hierarchy of log message importance. Messages below the
 * configured log level are filtered out to reduce noise and improve
 * performance in production builds.
 */
enum class LogLevel {
    Debug = 0,      ///< Detailed debug information for development
    Info = 1,       ///< General informational messages
    Warning = 2,    ///< Warning messages for potential issues
    Error = 3,      ///< Error messages for recoverable failures
    Critical = 4    ///< Critical errors that may cause termination
};

/**
 * @class Logger
 * @brief Thread-safe singleton logging system
 * 
 * Provides a centralized logging facility that can be safely used from multiple
 * threads. The logger supports both file and console output, configurable log
 * levels, and modern C++20 formatting for type-safe string formatting.
 * 
 * Thread safety:
 * - Uses std::mutex to protect all logging operations
 * - Safe to call from multiple threads simultaneously
 * - Atomic initialization using std::once_flag
 * 
 * Output destinations:
 * - File output: Persistent logging to a specified file
 * - Console output: Real-time display on stdout/stderr
 * - Both destinations can be enabled/disabled independently
 * 
 * Usage patterns:
 * 1. Call Initialize() once at application startup
 * 2. Use convenience macros (LOG_INFO, LOG_ERROR, etc.) for logging
 * 3. Configure log level to filter message verbosity
 * 4. Logger automatically cleans up on application exit
 */
class Logger {
private:
    // === Singleton Implementation ===
    static std::unique_ptr<Logger> instance;   ///< Singleton instance pointer
    static std::once_flag initFlag;            ///< Ensures single initialization
    
    // === Core Members ===
    std::ofstream logFile;                     ///< Output file stream for persistent logging
    std::mutex logMutex;                       ///< Mutex for thread-safe access
    LogLevel currentLevel;                     ///< Current minimum log level threshold
    bool consoleOutput;                        ///< Enable/disable console output

    /**
     * @brief Private constructor for singleton pattern
     * 
     * Initializes logger with default settings. Actual configuration
     * is done through the Initialize() method.
     */
    Logger();
    
    /**
     * @brief Generate timestamp string for log messages
     * 
     * Creates a formatted timestamp string using current system time.
     * Format: YYYY-MM-DD HH:MM:SS
     * 
     * @return Formatted timestamp string
     */
    std::string GetTimestamp() const;
    
    /**
     * @brief Convert log level enum to string representation
     * 
     * @param level LogLevel to convert
     * @return String representation of the log level
     */
    std::string LogLevelToString(LogLevel level) const;

public:
    // === Singleton Access ===
    
    /**
     * @brief Get the singleton logger instance
     * 
     * Thread-safe access to the global logger instance. Creates the instance
     * on first access using std::once_flag for thread safety.
     * 
     * @return Reference to the singleton Logger instance
     */
    static Logger& GetInstance();
    
    // === Configuration ===
    
    /**
     * @brief Initialize the logging system
     * 
     * Sets up the logger with specified configuration. Should be called once
     * at application startup before any logging operations.
     * 
     * @param filename Log file path (default: "particle_simulation.log")
     * @param level Minimum log level to output (default: Info)
     * @param enableConsole Enable console output (default: true)
     */
    void Initialize(const std::string& filename = "particle_simulation.log", 
                   LogLevel level = LogLevel::Info, 
                   bool enableConsole = true);
    
    /**
     * @brief Set minimum log level threshold
     * 
     * Messages below this level will be filtered out and not logged.
     * 
     * @param level New minimum log level
     */
    void SetLogLevel(LogLevel level) { currentLevel = level; }
    
    /**
     * @brief Enable or disable console output
     * 
     * @param enable true to enable console output, false to disable
     */
    void SetConsoleOutput(bool enable) { consoleOutput = enable; }
    
    // === Core Logging Method ===
    
    /**
     * @brief Generic logging method with C++20 formatting support
     * 
     * Thread-safe logging method that supports modern C++20 std::format
     * for type-safe string formatting. Automatically adds timestamp and
     * log level prefix to messages.
     * 
     * @tparam Args Variadic template for format arguments
     * @param level Severity level of the message
     * @param format Format string (std::format compatible)
     * @param args Arguments for format string
     */
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
    
    // === Convenience Methods for Each Log Level ===
    
    /**
     * @brief Log debug message with formatting support
     * 
     * @tparam Args Variadic template for format arguments
     * @param format Format string
     * @param args Arguments for format string
     */
    template<typename... Args>
    void Debug(const std::string& format, Args&&... args) {
        Log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }
    
    
    /**
     * @brief Log informational message with formatting support
     * 
     * @tparam Args Variadic template for format arguments
     * @param format Format string
     * @param args Arguments for format string
     */
    template<typename... Args>
    void Info(const std::string& format, Args&&... args) {
        Log(LogLevel::Info, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log warning message with formatting support
     * 
     * @tparam Args Variadic template for format arguments
     * @param format Format string
     * @param args Arguments for format string
     */
    template<typename... Args>
    void Warning(const std::string& format, Args&&... args) {
        Log(LogLevel::Warning, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log error message with formatting support
     * 
     * Error messages are automatically routed to stderr when console output is enabled.
     * 
     * @tparam Args Variadic template for format arguments
     * @param format Format string
     * @param args Arguments for format string
     */
    template<typename... Args>
    void Error(const std::string& format, Args&&... args) {
        Log(LogLevel::Error, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log critical message with formatting support
     * 
     * Critical messages are automatically routed to stderr when console output is enabled.
     * These represent severe errors that may cause application termination.
     * 
     * @tparam Args Variadic template for format arguments
     * @param format Format string
     * @param args Arguments for format string
     */
    template<typename... Args>
    void Critical(const std::string& format, Args&&... args) {
        Log(LogLevel::Critical, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Destructor
     * 
     * Ensures proper cleanup of file resources and flushes any pending output.
     */
    ~Logger();
};

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

/**
 * @brief Convenience macro for debug logging
 * 
 * Provides easy access to debug logging without having to call GetInstance().
 * Supports C++20 formatting with variadic arguments.
 * 
 * Usage: LOG_DEBUG("Processing {} particles at frame {}", count, frame);
 */
#define LOG_DEBUG(...) Logger::GetInstance().Debug(__VA_ARGS__)

/**
 * @brief Convenience macro for informational logging
 * 
 * Usage: LOG_INFO("Simulation initialized with {} particles", particleCount);
 */
#define LOG_INFO(...) Logger::GetInstance().Info(__VA_ARGS__)

/**
 * @brief Convenience macro for warning logging
 * 
 * Usage: LOG_WARNING("Performance degraded: frame time {}ms", frameTime);
 */
#define LOG_WARNING(...) Logger::GetInstance().Warning(__VA_ARGS__)

/**
 * @brief Convenience macro for error logging
 * 
 * Usage: LOG_ERROR("Failed to create buffer: {}", errorMessage);
 */
#define LOG_ERROR(...) Logger::GetInstance().Error(__VA_ARGS__)

/**
 * @brief Convenience macro for critical error logging
 * 
 * Usage: LOG_CRITICAL("DirectX device lost, attempting recovery");
 */
#define LOG_CRITICAL(...) Logger::GetInstance().Critical(__VA_ARGS__)

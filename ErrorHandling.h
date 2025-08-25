#pragma once

#include "Utilities.h"
#include "Logger.h"
#include <functional>
#include <vector>
#include <memory>
#include <map>
#include <chrono>

/**
 * @file ErrorHandling.h
 * @brief Comprehensive error handling, recovery, and quality management system
 * 
 * This file provides a robust error handling framework for DirectX applications,
 * including automatic error logging, device recovery mechanisms, and adaptive
 * quality management to maintain application stability.
 * 
 * Key features:
 * - Centralized error handling with detailed context tracking
 * - Automatic device lost detection and recovery
 * - Adaptive quality management for performance optimization
 * - Comprehensive logging and error history
 * - Graceful degradation under adverse conditions
 * 
 * The system is designed to handle common DirectX failure scenarios:
 * - Device removal/reset (GPU driver crashes, TDR events)
 * - Out-of-memory conditions
 * - Resource creation failures
 * - Performance degradation
 */

/**
 * @enum ErrorSeverity
 * @brief Classification of error severity levels
 * 
 * Used to categorize errors by their impact and required response level.
 * This enables appropriate filtering, logging, and recovery strategies.
 */
enum class ErrorSeverity {
    Info,       ///< Informational message, no action required
    Warning,    ///< Non-critical issue, application can continue
    Error,      ///< Serious error, may cause instability
    Critical    ///< Fatal error, application should terminate gracefully
};

/**
 * @struct ErrorContext
 * @brief Complete context information for an error occurrence
 * 
 * Captures all relevant information about an error event, enabling detailed
 * debugging, logging, and analysis of failure patterns. The context includes
 * both technical details (HRESULT, location) and metadata (timestamp, severity).
 */
struct ErrorContext {
    std::string function;                           ///< Function where error occurred
    std::string file;                              ///< Source file containing the error
    int line;                                      ///< Line number of error location
    HRESULT hr;                                    ///< DirectX HRESULT error code
    std::string message;                           ///< Human-readable error description
    ErrorSeverity severity;                        ///< Error severity classification
    std::chrono::steady_clock::time_point timestamp; ///< Time when error occurred
};

/**
 * @class ErrorHandler
 * @brief Centralized error handling and logging system
 * 
 * Singleton class that provides comprehensive error handling capabilities for
 * DirectX applications. It maintains error history, supports callback registration
 * for custom error handling, and provides analysis tools for debugging.
 * 
 * Key features:
 * - Centralized error processing and logging
 * - Customizable callback system for application-specific handling
 * - Historical error tracking with configurable limits
 * - Statistical analysis of error patterns
 * - Thread-safe operation for multi-threaded applications
 * 
 * Usage pattern:
 * 1. Register callbacks for custom error handling
 * 2. Use error checking macros throughout application
 * 3. Analyze error patterns through provided statistics
 * 4. Implement recovery strategies based on error context
 */
class ErrorHandler {
private:
    std::vector<ErrorContext> errorHistory;         ///< Historical log of all errors
    std::vector<std::function<void(const ErrorContext&)>> errorCallbacks; ///< Custom error handlers
    size_t maxHistorySize = 1000;                  ///< Maximum errors to keep in history
    
    static ErrorHandler instance;                   ///< Singleton instance

public:
    /**
     * @brief Get singleton instance of ErrorHandler
     * @return Reference to the global ErrorHandler instance
     */
    static ErrorHandler& GetInstance() { return instance; }
    
    /**
     * @brief Register a callback for custom error handling
     * 
     * Allows applications to register custom handlers that will be called
     * whenever an error occurs. Multiple callbacks can be registered.
     * 
     * @param callback Function to call when errors occur
     */
    void RegisterCallback(std::function<void(const ErrorContext&)> callback);
    
    /**
     * @brief Process and handle an error context
     * 
     * Central error processing function that logs the error, calls registered
     * callbacks, and maintains error history.
     * 
     * @param context Complete error context information
     */
    void HandleError(const ErrorContext& context);
    
    /**
     * @brief Log an error with complete context information
     * 
     * Convenience function for creating and handling error contexts from
     * DirectX HRESULT values and source location information.
     * 
     * @param hr DirectX HRESULT error code
     * @param message Human-readable error description
     * @param function Name of function where error occurred
     * @param file Source file containing error
     * @param line Line number of error location
     */
    void LogError(HRESULT hr, const std::string& message, 
                  const std::string& function, const std::string& file, int line);
    
    /**
     * @brief Get the most recent errors from history
     * 
     * @param count Number of recent errors to retrieve
     * @return Vector of recent error contexts, newest first
     */
    std::vector<ErrorContext> GetRecentErrors(size_t count = 10) const;
    
    /**
     * @brief Clear all error history
     * 
     * Removes all stored error contexts from history. Useful for
     * resetting after handling a batch of related errors.
     */
    void ClearHistory();
    
    // === Statistical Analysis ===
    
    /**
     * @brief Get count of errors by severity level
     * 
     * @param severity Severity level to count
     * @return Number of errors at the specified severity level
     */
    size_t GetErrorCount(ErrorSeverity severity) const;
    
    /**
     * @brief Print comprehensive error statistics to log
     * 
     * Outputs a detailed summary of error patterns, frequencies,
     * and trends to help with debugging and optimization.
     */
    void PrintErrorSummary() const;
};

// ============================================================================
// ENHANCED ERROR CHECKING MACROS
// ============================================================================

/**
 * @brief Enhanced HRESULT checking macro that throws on failure
 * 
 * This macro checks a DirectX HRESULT and throws a D3DException if it indicates
 * failure. It automatically captures source location and creates detailed error
 * context for debugging.
 * 
 * @param hr HRESULT value to check
 * @param msg Human-readable description of the operation that failed
 * 
 * Usage: CHECK_HR_ENHANCED(device->CreateBuffer(...), "Failed to create vertex buffer");
 */
#define CHECK_HR_ENHANCED(hr, msg) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            ErrorContext ctx; \
            ctx.function = __FUNCTION__; \
            ctx.file = __FILE__; \
            ctx.line = __LINE__; \
            ctx.hr = _hr; \
            ctx.message = msg; \
            ctx.severity = ErrorSeverity::Error; \
            ctx.timestamp = std::chrono::steady_clock::now(); \
            ErrorHandler::GetInstance().HandleError(ctx); \
            throw D3DException(_hr, msg); \
        } \
    } while(0)

/**
 * @brief HRESULT checking macro that logs warnings but doesn't throw
 * 
 * Similar to CHECK_HR_ENHANCED but treats failures as warnings that don't
 * interrupt program flow. Useful for non-critical operations.
 * 
 * @param hr HRESULT value to check
 * @param msg Human-readable description of the operation
 */
#define CHECK_HR_WARNING(hr, msg) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            ErrorContext ctx; \
            ctx.function = __FUNCTION__; \
            ctx.file = __FILE__; \
            ctx.line = __LINE__; \
            ctx.hr = _hr; \
            ctx.message = msg; \
            ctx.severity = ErrorSeverity::Warning; \
            ctx.timestamp = std::chrono::steady_clock::now(); \
            ErrorHandler::GetInstance().HandleError(ctx); \
        } \
    } while(0)

/**
 * @brief HRESULT logging macro for informational purposes
 * 
 * Logs HRESULT failures as informational messages. Used for operations
 * where failure is expected or acceptable in certain conditions.
 * 
 * @param hr HRESULT value to check
 * @param msg Human-readable description of the operation
 */
#define LOG_HR_INFO(hr, msg) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            ErrorContext ctx; \
            ctx.function = __FUNCTION__; \
            ctx.file = __FILE__; \
            ctx.line = __LINE__; \
            ctx.hr = _hr; \
            ctx.message = msg; \
            ctx.severity = ErrorSeverity::Info; \
            ctx.timestamp = std::chrono::steady_clock::now(); \
            ErrorHandler::GetInstance().HandleError(ctx); \
        } \
    } while(0)

/**
 * @class DeviceRecovery
 * @brief Automatic device lost detection and recovery system
 * 
 * Handles DirectX device lost scenarios (GPU driver crashes, TDR events, etc.)
 * by providing automatic detection and recovery mechanisms. This is critical
 * for maintaining application stability in adverse conditions.
 * 
 * Common device lost scenarios:
 * - GPU driver crashes or updates
 * - TDR (Timeout Detection and Recovery) events
 * - System sleep/resume cycles
 * - Multiple monitor configuration changes
 * - GPU overheating protection
 * 
 * Recovery strategy:
 * 1. Detect device lost condition
 * 2. Release all device-dependent resources
 * 3. Recreate device and resources
 * 4. Restore application state
 */
class DeviceRecovery {
private:
    bool deviceLost = false;                        ///< Current device lost status
    std::vector<std::function<void()>> recoveryCallbacks; ///< Recovery operation callbacks
    std::chrono::steady_clock::time_point lastRecoveryAttempt; ///< Time of last recovery attempt
    int recoveryAttempts = 0;                      ///< Number of recovery attempts made
    
    static constexpr int MAX_RECOVERY_ATTEMPTS = 3; ///< Maximum recovery attempts before giving up
    static constexpr auto RECOVERY_COOLDOWN = std::chrono::seconds(5); ///< Cooldown between attempts

public:
    /**
     * @brief Register a callback for device recovery operations
     * 
     * Applications should register callbacks that recreate device-dependent
     * resources like buffers, textures, shaders, and render targets.
     * 
     * @param callback Function to call during recovery process
     */
    void RegisterRecoveryCallback(std::function<void()> callback);
    
    /**
     * @brief Attempt to recover from device lost condition
     * 
     * Executes all registered recovery callbacks in order. Returns true
     * if recovery was successful, false if recovery failed or max
     * attempts exceeded.
     * 
     * @return true if recovery succeeded, false otherwise
     */
    bool AttemptRecovery();
    
    /**
     * @brief Check if device is currently in lost state
     * @return true if device is lost, false if operational
     */
    bool IsDeviceLost() const { return deviceLost; }
    
    /**
     * @brief Set device lost status
     * 
     * Called by the system when device lost conditions are detected
     * or when recovery is completed.
     * 
     * @param lost true if device is lost, false if recovered
     */
    void SetDeviceLost(bool lost) { deviceLost = lost; }
    
    /**
     * @brief Reset recovery attempt counter
     * 
     * Called after successful recovery or when starting fresh
     * recovery attempts after a period of stability.
     */
    void ResetRecoveryAttempts() { recoveryAttempts = 0; }
    
    /**
     * @brief Get current number of recovery attempts
     * @return Number of recovery attempts made
     */
    int GetRecoveryAttempts() const { return recoveryAttempts; }
    
    /**
     * @brief Check if recovery can be attempted
     * 
     * Considers cooldown period and maximum attempt limits to prevent
     * excessive recovery attempts that could cause system instability.
     * 
     * @return true if recovery attempt is allowed, false otherwise
     */
    bool CanAttemptRecovery() const;
};

/**
 * @class QualityManager
 * @brief Adaptive quality management and graceful degradation system
 * 
 * Manages rendering quality levels to maintain smooth performance under
 * varying system conditions. Provides both manual quality control and
 * automatic adaptation based on performance metrics.
 * 
 * Key features:
 * - Multiple predefined quality presets
 * - Automatic quality adjustment based on frame time
 * - Graceful degradation under performance pressure
 * - Per-feature quality control (particles, effects, resolution)
 * - Performance target maintenance
 * 
 * Quality adaptation strategy:
 * 1. Monitor frame time and performance metrics
 * 2. Detect performance issues or improvements
 * 3. Adjust quality level appropriately
 * 4. Apply new settings to rendering pipeline
 * 5. Continue monitoring for further adjustments
 */
class QualityManager {
public:
    /**
     * @enum QualityLevel
     * @brief Available rendering quality levels
     * 
     * Defines discrete quality levels from maximum visual fidelity down
     * to minimum playable quality. Each level represents a balance between
     * visual quality and performance requirements.
     */
    enum class QualityLevel {
        Ultra,      ///< Maximum quality, all features enabled
        High,       ///< High quality with minor compromises  
        Medium,     ///< Balanced quality and performance
        Low,        ///< Performance priority, visual compromises
        Minimum     ///< Minimum playable quality, maximum performance
    };
    
private:
    QualityLevel currentLevel = QualityLevel::High; ///< Current active quality level
    bool autoAdjust = true;                        ///< Enable automatic quality adjustment
    
    /**
     * @struct QualitySettings
     * @brief Complete quality configuration for a specific level
     * 
     * Defines all rendering parameters that change between quality levels.
     * This allows for granular control over performance vs. quality trade-offs.
     */
    struct QualitySettings {
        size_t maxParticles;            ///< Maximum particles to simulate/render
        bool enableGeometryShader;      ///< Enable geometry shader effects
        float renderScale;              ///< Render resolution scale factor
        int msaaSamples;               ///< MSAA sample count (0=disabled)
        bool enablePostProcessing;      ///< Enable post-processing effects
    };
    
    std::map<QualityLevel, QualitySettings> qualityPresets; ///< Predefined quality configurations

public:
    /**
     * @brief Initialize quality presets with default values
     * 
     * Sets up the predefined quality configurations for each quality level.
     * Should be called during application initialization.
     */
    void InitializePresets();
    
    /**
     * @brief Manually adjust quality to specific level
     * 
     * Forces quality to a specific level, disabling automatic adjustment
     * until re-enabled. Applies the new settings immediately.
     * 
     * @param level Target quality level
     */
    void AdjustQuality(QualityLevel level);
    
    /**
     * @brief Automatically adjust quality based on performance
     * 
     * Analyzes current frame time and adjusts quality level to maintain
     * target performance. Called each frame when auto-adjustment is enabled.
     * 
     * @param frameTime Current frame time in seconds
     */
    void AutoAdjustQuality(float frameTime);
    
    /**
     * @brief Get current active quality level
     * @return Current quality level
     */
    QualityLevel GetCurrentLevel() const { return currentLevel; }
    
    /**
     * @brief Get settings for current quality level
     * @return Quality settings structure for current level
     */
    QualitySettings GetCurrentSettings() const;
    
    /**
     * @brief Enable or disable automatic quality adjustment
     * @param enable true to enable auto-adjustment, false to disable
     */
    void EnableAutoAdjust(bool enable) { autoAdjust = enable; }
    
    /**
     * @brief Check if automatic quality adjustment is enabled
     * @return true if auto-adjustment is enabled, false otherwise
     */
    bool IsAutoAdjustEnabled() const { return autoAdjust; }
};

#pragma once

#include "Utilities.h"
#include "Logger.h"
#include <functional>
#include <vector>
#include <memory>

// Modern DirectX error handling system

enum class ErrorSeverity {
    Info,
    Warning,
    Error,
    Critical
};

struct ErrorContext {
    std::string function;
    std::string file;
    int line;
    HRESULT hr;
    std::string message;
    ErrorSeverity severity;
    std::chrono::steady_clock::time_point timestamp;
};

class ErrorHandler {
private:
    std::vector<ErrorContext> errorHistory;
    std::vector<std::function<void(const ErrorContext&)>> errorCallbacks;
    size_t maxHistorySize = 1000;
    
    static ErrorHandler instance;

public:
    static ErrorHandler& GetInstance() { return instance; }
    
    void RegisterCallback(std::function<void(const ErrorContext&)> callback);
    void HandleError(const ErrorContext& context);
    
    void LogError(HRESULT hr, const std::string& message, 
                  const std::string& function, const std::string& file, int line);
    
    std::vector<ErrorContext> GetRecentErrors(size_t count = 10) const;
    void ClearHistory();
    
    // Statistics
    size_t GetErrorCount(ErrorSeverity severity) const;
    void PrintErrorSummary() const;
};

// Enhanced error checking macros
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

// Device lost recovery system
class DeviceRecovery {
private:
    bool deviceLost = false;
    std::vector<std::function<void()>> recoveryCallbacks;
    std::chrono::steady_clock::time_point lastRecoveryAttempt;
    int recoveryAttempts = 0;
    
    static constexpr int MAX_RECOVERY_ATTEMPTS = 3;
    static constexpr auto RECOVERY_COOLDOWN = std::chrono::seconds(5);

public:
    void RegisterRecoveryCallback(std::function<void()> callback);
    bool AttemptRecovery();
    bool IsDeviceLost() const { return deviceLost; }
    void SetDeviceLost(bool lost) { deviceLost = lost; }
    void ResetRecoveryAttempts() { recoveryAttempts = 0; }
    
    int GetRecoveryAttempts() const { return recoveryAttempts; }
    bool CanAttemptRecovery() const;
};

// Graceful degradation system
class QualityManager {
public:
    enum class QualityLevel {
        Ultra,
        High, 
        Medium,
        Low,
        Minimum
    };
    
private:
    QualityLevel currentLevel = QualityLevel::High;
    bool autoAdjust = true;
    
    struct QualitySettings {
        size_t maxParticles;
        bool enableGeometryShader;
        float renderScale;
        int msaaSamples;
        bool enablePostProcessing;
    };
    
    std::map<QualityLevel, QualitySettings> qualityPresets;

public:
    void InitializePresets();
    void AdjustQuality(QualityLevel level);
    void AutoAdjustQuality(float frameTime);
    
    QualityLevel GetCurrentLevel() const { return currentLevel; }
    QualitySettings GetCurrentSettings() const;
    
    void EnableAutoAdjust(bool enable) { autoAdjust = enable; }
    bool IsAutoAdjustEnabled() const { return autoAdjust; }
};

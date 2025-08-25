/**
 * @file Utilities.cpp
 * @brief Implementation of utility functions and helper classes
 * 
 * This file provides common utility functions used throughout the particle simulation
 * system, including error handling, string formatting, and platform-specific operations.
 * 
 * Key features:
 * - DirectX error handling and HRESULT formatting
 * - Human-readable size formatting for memory/performance statistics  
 * - String conversion utilities for Windows API interoperability
 * - Cross-platform compatibility helpers
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#include "Utilities.h"
#include <array>

/**
 * @brief Constructs a DirectX exception with formatted error message
 * 
 * Creates a D3DException with a detailed error message that includes both
 * the custom message and the system-formatted HRESULT error description.
 * 
 * @param hr HRESULT error code from DirectX operation
 * @param message Custom error message describing the operation context
 */
D3DException::D3DException(HRESULT hr, const std::string& message) 
    : std::runtime_error(MakeErrorString(hr, message)), hr(hr) {}

/**
 * @brief Formats an HRESULT error code into a human-readable error message
 * 
 * Uses the Windows FormatMessage API to convert HRESULT codes into descriptive
 * error text, combined with custom context messages for better debugging.
 * 
 * @param hr HRESULT error code to format
 * @param message Custom message providing operation context
 * @return Formatted error string with both custom and system error descriptions
 */
std::string D3DException::MakeErrorString(HRESULT hr, const std::string& message) {
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr
    );

    std::string errorMsg(messageBuffer, size);
    LocalFree(messageBuffer);
    
    // Remove trailing whitespace for cleaner error messages
    errorMsg.erase(errorMsg.find_last_not_of(" \t\n\r\f\v") + 1);
    
    return std::format("{} {} HRESULT: 0x{:08X}", message, errorMsg, static_cast<unsigned long>(hr));
}

namespace Utils {
    /**
     * @brief Converts byte sizes into human-readable format with appropriate units
     * 
     * Automatically selects the most appropriate unit (B, KB, MB, GB, TB) and
     * formats the size with decimal precision for readability in statistics
     * and memory usage displays.
     * 
     * @param size Size in bytes to convert
     * @return Formatted string with size and unit (e.g., "1.23 MB", "512 KB")
     */
    std::string HumanReadableSize(uint64_t size) {
        constexpr std::array<std::pair<const char*, uint64_t>, 5> units = {{
            {"B", 1},
            {"KB", 1024},
            {"MB", 1024 * 1024},
            {"GB", 1024 * 1024 * 1024},
            {"TB", 1024ULL * 1024 * 1024 * 1024}
        }};

        // Find the largest unit that the size fits into
        for (auto it = units.rbegin(); it != units.rend(); ++it) {
            if (size >= it->second) {
                return std::format("{:.2f}{}", static_cast<double>(size) / it->second, it->first);
            }
        }
        return std::format("{}B", size);
    }

    /**
     * @brief Converts wide character strings to UTF-8 narrow strings
     * 
     * Provides conversion from Windows wide character strings (wstring) to
     * UTF-8 encoded narrow strings for cross-platform compatibility and
     * API interoperability.
     * 
     * @param wideString Wide character string to convert  
     * @return UTF-8 encoded narrow string, empty if input is empty
     */
    std::string ConvertWideToNarrow(const std::wstring& wideString) {
        if (wideString.empty()) return std::string();

        // Calculate required buffer size for conversion
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wideString[0], 
                                            static_cast<int>(wideString.size()), 
                                            nullptr, 0, nullptr, nullptr);
        std::string narrowString(size_needed, 0);
        
        // Perform the actual conversion from wide to UTF-8
        WideCharToMultiByte(CP_UTF8, 0, &wideString[0], static_cast<int>(wideString.size()),
                           &narrowString[0], size_needed, nullptr, nullptr);
        return narrowString;
    }
}

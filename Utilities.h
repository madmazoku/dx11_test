/**
 * @file Utilities.h
 * @brief Header for utility functions and helper classes used throughout the particle system
 * 
 * This file provides essential utility functions and classes for error handling,
 * string formatting, type conversions, and platform-specific operations. It includes
 * specialized DirectX error handling and common formatting utilities.
 * 
 * Key features:
 * - D3DException class for DirectX error handling with detailed HRESULT formatting
 * - THROW_IF_FAILED macro for streamlined error checking in DirectX calls
 * - String conversion utilities for Windows API interoperability
 * - Type-safe size conversion functions for DirectX API compatibility
 * - Human-readable formatting for sizes and performance statistics
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#pragma once

#include <stdexcept>
#include <string>
#include <format>
#include <Windows.h>

/**
 * @class D3DException
 * @brief Specialized exception class for DirectX error handling
 * 
 * Extends std::runtime_error to provide detailed error information for DirectX
 * operations. Automatically formats HRESULT codes into human-readable error
 * messages using the Windows FormatMessage API.
 * 
 * The exception preserves the original HRESULT code for programmatic handling
 * while providing descriptive error messages for debugging and logging.
 */
class D3DException : public std::runtime_error {
private:
    HRESULT hr;  ///< The DirectX HRESULT error code that triggered this exception
    
public:
    /**
     * @brief Constructs a DirectX exception with HRESULT and context message
     * @param hr HRESULT error code from DirectX operation
     * @param message Custom error message describing the operation context
     */
    D3DException(HRESULT hr, const std::string& message);
    
    /**
     * @brief Gets the original HRESULT error code
     * @return HRESULT code that caused this exception
     */
    HRESULT GetHResult() const { return hr; }
    
private:
    /**
     * @brief Creates formatted error string from HRESULT and message
     * @param hr HRESULT error code to format
     * @param message Custom context message
     * @return Combined formatted error string
     */
    static std::string MakeErrorString(HRESULT hr, const std::string& message);
};

/**
 * @brief Utility macro for streamlined DirectX error checking
 * 
 * Evaluates an HRESULT-returning expression and throws D3DException if it failed.
 * Provides consistent error handling across all DirectX API calls with minimal
 * code duplication.
 * 
 * @param hr Expression that returns an HRESULT
 * @param msg Error message to include if the operation fails
 * 
 * Usage example:
 * @code
 * THROW_IF_FAILED(device->CreateBuffer(&desc, nullptr, buffer.getAddressOf()), 
 *                 "Failed to create vertex buffer");
 * @endcode
 */
#define THROW_IF_FAILED(hr, msg) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            throw D3DException(_hr, msg); \
        } \
    } while(0)

/**
 * @namespace Utils
 * @brief Collection of utility functions for common operations
 * 
 * Provides helper functions for string formatting, type conversions,
 * and platform-specific operations used throughout the particle system.
 */
namespace Utils {
    /**
     * @brief Converts byte sizes into human-readable format with units
     * @param size Size in bytes to convert
     * @return Formatted string with appropriate unit (B, KB, MB, GB, TB)
     */
    std::string HumanReadableSize(uint64_t size);
    
    /**
     * @brief Converts Windows wide character strings to UTF-8 narrow strings  
     * @param wideString Wide character string to convert
     * @return UTF-8 encoded narrow string
     */
    std::string ConvertWideToNarrow(const std::wstring& wideString);
    
    /**
     * @brief Safely converts integral types to UINT with overflow checking
     * 
     * Template function that performs range-checked conversion from any integral
     * type to UINT, throwing an exception if the value exceeds UINT_MAX. This
     * prevents silent overflow bugs when interfacing with DirectX APIs.
     * 
     * @tparam T Any integral type to convert from
     * @param size Value to convert to UINT
     * @return Safely converted UINT value
     * @throws std::out_of_range if size exceeds UINT_MAX
     */
    template<typename T>
    constexpr UINT SafeSizeTToUINT(T size) {
        static_assert(std::is_integral_v<T>);
        if (size > UINT_MAX) {
            throw std::out_of_range(std::format("Size {} exceeds UINT_MAX", size));
        }
        return static_cast<UINT>(size);
    }
}

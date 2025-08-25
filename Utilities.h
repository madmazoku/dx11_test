#pragma once

#include <stdexcept>
#include <string>
#include <format>
#include <Windows.h>

class D3DException : public std::runtime_error {
private:
    HRESULT hr;
    
public:
    D3DException(HRESULT hr, const std::string& message);
    HRESULT GetHResult() const { return hr; }
    
private:
    static std::string MakeErrorString(HRESULT hr, const std::string& message);
};

// Utility macro for error checking
#define THROW_IF_FAILED(hr, msg) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            throw D3DException(_hr, msg); \
        } \
    } while(0)

// Utility functions
namespace Utils {
    std::string HumanReadableSize(uint64_t size);
    std::string ConvertWideToNarrow(const std::wstring& wideString);
    
    template<typename T>
    constexpr UINT SafeSizeTToUINT(T size) {
        static_assert(std::is_integral_v<T>);
        if (size > UINT_MAX) {
            throw std::out_of_range(std::format("Size {} exceeds UINT_MAX", size));
        }
        return static_cast<UINT>(size);
    }
}

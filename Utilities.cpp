#include "Utilities.h"
#include <array>

D3DException::D3DException(HRESULT hr, const std::string& message) 
    : std::runtime_error(MakeErrorString(hr, message)), hr(hr) {}

std::string D3DException::MakeErrorString(HRESULT hr, const std::string& message) {
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr
    );

    std::string errorMsg(messageBuffer, size);
    LocalFree(messageBuffer);
    
    // Remove trailing whitespace
    errorMsg.erase(errorMsg.find_last_not_of(" \t\n\r\f\v") + 1);
    
    return std::format("{} {} HRESULT: 0x{:08X}", message, errorMsg, static_cast<unsigned long>(hr));
}

namespace Utils {
    std::string HumanReadableSize(uint64_t size) {
        constexpr std::array<std::pair<const char*, uint64_t>, 5> units = {{
            {"B", 1},
            {"KB", 1024},
            {"MB", 1024 * 1024},
            {"GB", 1024 * 1024 * 1024},
            {"TB", 1024ULL * 1024 * 1024 * 1024}
        }};

        for (auto it = units.rbegin(); it != units.rend(); ++it) {
            if (size >= it->second) {
                return std::format("{:.2f}{}", static_cast<double>(size) / it->second, it->first);
            }
        }
        return std::format("{}B", size);
    }

    std::string ConvertWideToNarrow(const std::wstring& wideString) {
        if (wideString.empty()) return std::string();

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wideString[0], 
                                            static_cast<int>(wideString.size()), 
                                            nullptr, 0, nullptr, nullptr);
        std::string narrowString(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wideString[0], static_cast<int>(wideString.size()),
                           &narrowString[0], size_needed, nullptr, nullptr);
        return narrowString;
    }
}

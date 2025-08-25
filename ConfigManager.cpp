#include "ConfigManager.h"
#include "Logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>

bool ConfigManager::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_WARNING("Could not open config file: {}", filename);
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    try {
        ParseJson(content);
        LOG_INFO("Configuration loaded from: {}", filename);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config file {}: {}", filename, e.what());
        return false;
    }
}

void ConfigManager::ParseJson(const std::string& content) {
    // Simple JSON parser - only handles basic structure we need
    // For production, use a proper JSON library like nlohmann/json
    
    std::string cleanContent = content;
    // Remove whitespace and line breaks
    cleanContent.erase(std::remove_if(cleanContent.begin(), cleanContent.end(),
        [](char c) { return std::isspace(c) && c != ' '; }), cleanContent.end());
    
    size_t pos = 0;
    std::string currentPath;
    
    while (pos < cleanContent.length()) {
        size_t keyStart = cleanContent.find('"', pos);
        if (keyStart == std::string::npos) break;
        
        size_t keyEnd = cleanContent.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;
        
        std::string key = cleanContent.substr(keyStart + 1, keyEnd - keyStart - 1);
        
        size_t colonPos = cleanContent.find(':', keyEnd);
        if (colonPos == std::string::npos) break;
        
        pos = colonPos + 1;
        
        // Skip whitespace
        while (pos < cleanContent.length() && std::isspace(cleanContent[pos])) pos++;
        
        if (pos >= cleanContent.length()) break;
        
        if (cleanContent[pos] == '{') {
            // Start of nested object
            currentPath = key;
            pos++;
        }
        else if (cleanContent[pos] == '[') {
            // Array - find closing bracket and extract values
            size_t arrayEnd = cleanContent.find(']', pos);
            if (arrayEnd != std::string::npos) {
                std::string arrayStr = cleanContent.substr(pos + 1, arrayEnd - pos - 1);
                SetNestedValue(currentPath.empty() ? key : currentPath + "." + key, arrayStr);
                pos = arrayEnd + 1;
            }
        }
        else if (cleanContent[pos] == '"') {
            // String value
            size_t valueStart = pos + 1;
            size_t valueEnd = cleanContent.find('"', valueStart);
            if (valueEnd != std::string::npos) {
                std::string value = cleanContent.substr(valueStart, valueEnd - valueStart);
                SetNestedValue(currentPath.empty() ? key : currentPath + "." + key, value);
                pos = valueEnd + 1;
            }
        }
        else {
            // Number or boolean
            size_t valueStart = pos;
            size_t valueEnd = cleanContent.find_first_of(",}]", pos);
            if (valueEnd != std::string::npos) {
                std::string value = cleanContent.substr(valueStart, valueEnd - valueStart);
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                SetNestedValue(currentPath.empty() ? key : currentPath + "." + key, value);
                pos = valueEnd;
            }
        }
        
        // Reset current path on closing brace
        if (pos < cleanContent.length() && cleanContent[pos] == '}') {
            currentPath.clear();
            pos++;
        }
    }
}

void ConfigManager::SetNestedValue(const std::string& path, const std::string& value) {
    configValues[path] = value;
}

std::string ConfigManager::GetValue(const std::string& path, const std::string& defaultValue) const {
    auto it = configValues.find(path);
    return (it != configValues.end()) ? it->second : defaultValue;
}

std::string ConfigManager::GetString(const std::string& path, const std::string& defaultValue) const {
    return GetValue(path, defaultValue);
}

int ConfigManager::GetInt(const std::string& path, int defaultValue) const {
    std::string value = GetValue(path);
    if (value.empty()) return defaultValue;
    
    try {
        return std::stoi(value);
    }
    catch (...) {
        return defaultValue;
    }
}

float ConfigManager::GetFloat(const std::string& path, float defaultValue) const {
    std::string value = GetValue(path);
    if (value.empty()) return defaultValue;
    
    try {
        return std::stof(value);
    }
    catch (...) {
        return defaultValue;
    }
}

bool ConfigManager::GetBool(const std::string& path, bool defaultValue) const {
    std::string value = GetValue(path);
    if (value.empty()) return defaultValue;
    
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value == "true" || value == "1";
}

void ConfigManager::GetFloatArray(const std::string& path, float* output, size_t count) const {
    std::string value = GetValue(path);
    if (value.empty()) return;
    
    // Parse array format: "1.0,2.0,3.0" or similar
    std::stringstream ss(value);
    std::string item;
    size_t index = 0;
    
    while (std::getline(ss, item, ',') && index < count) {
        try {
            // Remove whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            output[index] = std::stof(item);
        }
        catch (...) {
            // Keep default value on parse error
        }
        index++;
    }
}

SimulationConfig ConfigManager::GetSimulationConfig() const {
    SimulationConfig config;
    
    config.particleCount = GetInt("simulation.particleCount", 128);
    config.timeStep = GetFloat("simulation.timeStep", 0.016f);
    config.damping = GetFloat("simulation.damping", 0.995f);
    config.springConstant = GetFloat("simulation.springConstant", 50.0f);
    config.restLength = GetFloat("simulation.restLength", 0.5f);
    config.maxFrames = GetInt("simulation.maxFrames", 7200);
    config.enableDebugOutput = GetBool("simulation.enableDebugOutput", true);
    config.debugOutputInterval = GetInt("simulation.debugOutputInterval", 60);
    
    GetFloatArray("simulation.gravity", config.gravity, 3);
    GetFloatArray("simulation.boundaryMin", config.boundaryMin, 3);
    GetFloatArray("simulation.boundaryMax", config.boundaryMax, 3);
    
    return config;
}

RenderConfig ConfigManager::GetRenderConfig() const {
    RenderConfig config;
    
    config.windowWidth = GetInt("rendering.windowWidth", 1280);
    config.windowHeight = GetInt("rendering.windowHeight", 720);
    config.enableVSync = GetBool("rendering.enableVSync", true);
    config.sphereRadius = GetFloat("rendering.sphereRadius", 0.1f);
    
    GetFloatArray("rendering.clearColor", config.clearColor, 4);
    
    // Camera settings
    config.cameraRadius = GetFloat("rendering.camera.radius", 12.0f);
    config.cameraHeight = GetFloat("rendering.camera.height", 5.0f);
    config.cameraRotationSpeed = GetFloat("rendering.camera.rotationSpeed", 0.5f);
    config.cameraFov = GetFloat("rendering.camera.fov", 45.0f);
    config.cameraNearPlane = GetFloat("rendering.camera.nearPlane", 0.1f);
    config.cameraFarPlane = GetFloat("rendering.camera.farPlane", 100.0f);
    
    // Lighting settings
    GetFloatArray("rendering.lighting.direction", config.lightDirection, 3);
    GetFloatArray("rendering.lighting.color", config.lightColor, 3);
    config.lightIntensity = GetFloat("rendering.lighting.intensity", 1.0f);
    config.ambientIntensity = GetFloat("rendering.lighting.ambientIntensity", 0.2f);
    config.specularPower = GetFloat("rendering.lighting.specularPower", 32.0f);
    
    return config;
}

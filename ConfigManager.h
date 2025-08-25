#pragma once

#include "Structures.h"
#include <string>
#include <fstream>
#include <map>

class ConfigManager {
private:
    std::map<std::string, std::string> configValues;
    
    void ParseJson(const std::string& content);
    void SetNestedValue(const std::string& path, const std::string& value);
    std::string GetValue(const std::string& path, const std::string& defaultValue = "") const;
    
    template<typename T>
    T ParseValue(const std::string& value) const;

public:
    bool LoadFromFile(const std::string& filename);
    
    SimulationConfig GetSimulationConfig() const;
    RenderConfig GetRenderConfig() const;
    
    // Generic getters for different types
    std::string GetString(const std::string& path, const std::string& defaultValue = "") const;
    int GetInt(const std::string& path, int defaultValue = 0) const;
    float GetFloat(const std::string& path, float defaultValue = 0.0f) const;
    bool GetBool(const std::string& path, bool defaultValue = false) const;
    
    // Array getters (for float3 vectors etc)
    void GetFloatArray(const std::string& path, float* output, size_t count) const;
};

template<>
int ConfigManager::ParseValue<int>(const std::string& value) const;

template<>
float ConfigManager::ParseValue<float>(const std::string& value) const;

template<>
bool ConfigManager::ParseValue<bool>(const std::string& value) const;

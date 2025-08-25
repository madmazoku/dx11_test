#pragma once

#include "Structures.h"
#include <string>
#include <vector>
#include <map>

class ConfigManager {
private:
    std::map<std::string, std::string> configValues;
    
    // Configuration objects
    SimulationConfig simulationConfig;
    RenderConfig renderConfig;
    
    // Advanced JSON parsing methods
    void ParseJsonAdvanced(const std::string& content);
    void ParseSimulationSettings(const std::string& content);
    void ParseEnvironmentSettings(const std::string& content);
    void ParseParticleTypes(const std::string& content);
    void ParseInteractionRules(const std::string& content);
    void ParseTypeDistribution(const std::string& content);
    void ParseRenderingSettings(const std::string& content);
    void ParseCameraSettings(const std::string& renderBlock);
    void ParseLightingSettings(const std::string& renderBlock);
    void ParseCullingSettings(const std::string& renderBlock);
    void ParseIcosphereSettings(const std::string& renderBlock);
    
    // Type-specific parsing
    ParticleTypeInfo ParseParticleType(const std::string& typeObj);
    MaterialProperties ParseMaterial(const std::string& materialObj);
    InteractionRule ParseInteractionRule(const std::string& ruleObj);
    ForceParameters ParseForceParameters(const std::string& forceObj);
    
    // Utility parsing functions
    float ExtractFloatValue(const std::string& block, const std::string& key, float defaultValue);
    int ExtractIntValue(const std::string& block, const std::string& key, int defaultValue);
    bool ExtractBoolValue(const std::string& block, const std::string& key, bool defaultValue);
    std::string ExtractStringValue(const std::string& block, const std::string& key, const std::string& defaultValue);
    std::vector<float> ExtractFloat3Value(const std::string& block, const std::string& key, const std::vector<float>& defaultValue);
    std::vector<float> ExtractFloat4Value(const std::string& block, const std::string& key, const std::vector<float>& defaultValue);
    
    // Legacy simple JSON parsing for backward compatibility
    void ParseJson(const std::string& content);
    void SetNestedValue(const std::string& path, const std::string& value);
    std::string GetValue(const std::string& path, const std::string& defaultValue = "") const;
    
    template<typename T>
    T ParseValue(const std::string& value) const;

public:
    bool LoadFromFile(const std::string& filename);
    
    // Get configuration objects
    const SimulationConfig& GetSimulationConfig() const { return simulationConfig; }
    const RenderConfig& GetRenderConfig() const { return renderConfig; }
    
    // Legacy getters for backward compatibility
    std::string GetString(const std::string& path, const std::string& defaultValue = "") const;
    int GetInt(const std::string& path, int defaultValue = 0) const;
    float GetFloat(const std::string& path, float defaultValue = 0.0f) const;
    bool GetBool(const std::string& path, bool defaultValue = false) const;
    void GetFloatArray(const std::string& path, float* output, size_t count) const;
    
    // Multi-type system accessors
    size_t GetParticleTypeCount() const { return simulationConfig.particleTypes.size(); }
    const ParticleTypeInfo& GetParticleType(size_t index) const { return simulationConfig.particleTypes[index]; }
    const std::vector<ParticleTypeInfo>& GetParticleTypes() const { return simulationConfig.particleTypes; }
    
    size_t GetInteractionRuleCount() const { return simulationConfig.interactionRules.size(); }
    const InteractionRule& GetInteractionRule(size_t index) const { return simulationConfig.interactionRules[index]; }
    const std::vector<InteractionRule>& GetInteractionRules() const { return simulationConfig.interactionRules; }
    
    const std::vector<SimulationConfig::TypeDistribution>& GetTypeDistribution() const { return simulationConfig.typeDistribution; }
    
    // Environment and rendering accessors
    const EnvironmentConfig& GetEnvironment() const { return simulationConfig.environment; }
    const CullingConfig& GetCullingConfig() const { return renderConfig.culling; }
    const IcosphereConfig& GetIcosphereConfig() const { return renderConfig.icosphere; }
};

template<>
int ConfigManager::ParseValue<int>(const std::string& value) const;

template<>
float ConfigManager::ParseValue<float>(const std::string& value) const;

template<>
bool ConfigManager::ParseValue<bool>(const std::string& value) const;

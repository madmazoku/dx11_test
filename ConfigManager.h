#pragma once

#include "Structures.h"
#include <string>
#include <vector>
#include <map>

/**
 * @file ConfigManager.h
 * @brief Configuration file management and JSON parsing system
 * 
 * This file provides comprehensive configuration management for the particle
 * simulation system, supporting both simple key-value configurations and
 * complex multi-type particle system definitions through JSON parsing.
 * 
 * Key features:
 * - JSON-based configuration files with comprehensive parsing
 * - Multi-type particle system configuration
 * - Backward compatibility with legacy configuration formats
 * - Type-safe configuration value extraction
 * - Hierarchical configuration organization
 * - Error handling and validation
 * 
 * The ConfigManager supports two configuration paradigms:
 * 1. Legacy key-value access for simple configurations
 * 2. Modern structured objects for complex multi-type systems
 */

/**
 * @class ConfigManager
 * @brief Centralized configuration management system
 * 
 * Manages loading, parsing, and providing access to all configuration data
 * for the particle simulation system. Supports both legacy key-value access
 * and modern structured configuration objects.
 * 
 * Configuration workflow:
 * 1. Load JSON configuration file
 * 2. Parse into structured objects (SimulationConfig, RenderConfig)
 * 3. Provide type-safe access to configuration values
 * 4. Support runtime configuration queries and updates
 * 
 * The system is designed to be extensible, allowing new configuration
 * sections and parameters to be added without breaking existing code.
 */
class ConfigManager {
private:
    std::map<std::string, std::string> configValues;    ///< Legacy key-value storage
    
    // === Primary Configuration Objects ===
    SimulationConfig simulationConfig;     ///< Complete simulation configuration
    RenderConfig renderConfig;             ///< Complete rendering configuration
    
    // === Advanced JSON Parsing Methods ===
    // These methods parse specific sections of the JSON configuration
    
    /**
     * @brief Parse complete JSON configuration file
     * @param content Raw JSON content string
     */
    void ParseJsonAdvanced(const std::string& content);
    
    /**
     * @brief Parse simulation-specific settings
     * @param content JSON content containing simulation section
     */
    void ParseSimulationSettings(const std::string& content);
    
    /**
     * @brief Parse environmental force settings
     * @param content JSON content containing environment section
     */
    void ParseEnvironmentSettings(const std::string& content);
    
    /**
     * @brief Parse particle type definitions
     * @param content JSON content containing particleTypes array
     */
    void ParseParticleTypes(const std::string& content);
    
    /**
     * @brief Parse interaction rule definitions
     * @param content JSON content containing interactionRules array
     */
    void ParseInteractionRules(const std::string& content);
    
    /**
     * @brief Parse particle type distribution settings
     * @param content JSON content containing typeDistribution array
     */
    void ParseTypeDistribution(const std::string& content);
    
    /**
     * @brief Parse rendering system settings
     * @param content JSON content containing rendering section
     */
    void ParseRenderingSettings(const std::string& content);
    
    /**
     * @brief Parse camera configuration from rendering block
     * @param renderBlock JSON rendering section content
     */
    void ParseCameraSettings(const std::string& renderBlock);
    
    /**
     * @brief Parse lighting configuration from rendering block
     * @param renderBlock JSON rendering section content
     */
    void ParseLightingSettings(const std::string& renderBlock);
    
    /**
     * @brief Parse culling and LOD settings from rendering block
     * @param renderBlock JSON rendering section content
     */
    void ParseCullingSettings(const std::string& renderBlock);
    
    /**
     * @brief Parse icosphere rendering settings from rendering block
     * @param renderBlock JSON rendering section content
     */
    void ParseIcosphereSettings(const std::string& renderBlock);
    
    // === Type-Specific Parsing Methods ===
    
    /**
     * @brief Parse a single particle type definition
     * @param typeObj JSON object representing one particle type
     * @return Parsed ParticleType structure
     */
    ParticleType ParseParticleType(const std::string& typeObj);
    
    /**
     * @brief Parse material properties definition
     * @param materialObj JSON object representing material properties
     * @return Parsed MaterialProperties structure
     */
    MaterialProperties ParseMaterial(const std::string& materialObj);
    
    /**
     * @brief Parse a single interaction rule definition
     * @param ruleObj JSON object representing one interaction rule
     * @return Parsed InteractionRule structure
     */
    InteractionRule ParseInteractionRule(const std::string& ruleObj);
    
    /**
     * @brief Parse force descriptor parameters
     * @param forceObj JSON object representing force parameters
     * @return Parsed ForceDescriptor structure
     */
    ForceDescriptor ParseForceParameters(const std::string& forceObj);
    
    
    // === Utility JSON Parsing Functions ===
    // Low-level parsing utilities for extracting typed values from JSON strings
    
    /**
     * @brief Extract float value from JSON block
     * @param block JSON block to search in
     * @param key Key to look for
     * @param defaultValue Value to return if key not found
     * @return Parsed float value or default
     */
    float ExtractFloatValue(const std::string& block, const std::string& key, float defaultValue);
    
    /**
     * @brief Extract integer value from JSON block
     * @param block JSON block to search in
     * @param key Key to look for
     * @param defaultValue Value to return if key not found
     * @return Parsed integer value or default
     */
    int ExtractIntValue(const std::string& block, const std::string& key, int defaultValue);
    
    /**
     * @brief Extract boolean value from JSON block
     * @param block JSON block to search in
     * @param key Key to look for
     * @param defaultValue Value to return if key not found
     * @return Parsed boolean value or default
     */
    bool ExtractBoolValue(const std::string& block, const std::string& key, bool defaultValue);
    
    /**
     * @brief Extract string value from JSON block
     * @param block JSON block to search in
     * @param key Key to look for
     * @param defaultValue Value to return if key not found
     * @return Parsed string value or default
     */
    std::string ExtractStringValue(const std::string& block, const std::string& key, const std::string& defaultValue);
    
    /**
     * @brief Extract 3-component float array from JSON block
     * @param block JSON block to search in
     * @param key Key to look for
     * @param defaultValue Default 3-element vector to return if key not found
     * @return Vector containing 3 float values
     */
    std::vector<float> ExtractFloat3Value(const std::string& block, const std::string& key, const std::vector<float>& defaultValue);
    
    /**
     * @brief Extract 4-component float array from JSON block
     * @param block JSON block to search in
     * @param key Key to look for
     * @param defaultValue Default 4-element vector to return if key not found
     * @return Vector containing 4 float values
     */
    std::vector<float> ExtractFloat4Value(const std::string& block, const std::string& key, const std::vector<float>& defaultValue);
    
    // === Legacy JSON Parsing (Backward Compatibility) ===
    
    /**
     * @brief Legacy simple JSON parser for backward compatibility
     * @param content JSON content to parse using legacy format
     */
    void ParseJson(const std::string& content);
    
    /**
     * @brief Set nested configuration value using dot-notation path
     * @param path Hierarchical path (e.g., "rendering.camera.fov")
     * @param value String value to set
     */
    void SetNestedValue(const std::string& path, const std::string& value);
    
    /**
     * @brief Get configuration value using dot-notation path
     * @param path Hierarchical path to value
     * @param defaultValue Default value if path not found
     * @return Configuration value as string
     */
    std::string GetValue(const std::string& path, const std::string& defaultValue = "") const;
    
    /**
     * @brief Template function for parsing string values to specific types
     * @tparam T Target type for conversion
     * @param value String value to convert
     * @return Converted value of type T
     */
    template<typename T>
    T ParseValue(const std::string& value) const;

public:
    // === Primary Interface ===
    
    /**
     * @brief Load configuration from JSON file
     * 
     * Loads and parses a complete configuration file, populating both
     * the structured configuration objects and legacy key-value store.
     * 
     * @param filename Path to JSON configuration file
     * @return true if loading successful, false on error
     */
    bool LoadFromFile(const std::string& filename);
    
    // === Modern Configuration Object Access ===
    
    /**
     * @brief Get complete simulation configuration
     * @return Reference to SimulationConfig object
     */
    const SimulationConfig& GetSimulationConfig() const { return simulationConfig; }
    
    /**
     * @brief Get complete rendering configuration
     * @return Reference to RenderConfig object
     */
    const RenderConfig& GetRenderConfig() const { return renderConfig; }
    
    // === Legacy Configuration Access (Backward Compatibility) ===
    
    /**
     * @brief Get string configuration value by path
     * @param path Dot-notation path to value
     * @param defaultValue Default value if not found
     * @return Configuration value as string
     */
    std::string GetString(const std::string& path, const std::string& defaultValue = "") const;
    
    /**
     * @brief Get integer configuration value by path
     * @param path Dot-notation path to value
     * @param defaultValue Default value if not found
     * @return Configuration value as integer
     */
    int GetInt(const std::string& path, int defaultValue = 0) const;
    
    /**
     * @brief Get float configuration value by path
     * @param path Dot-notation path to value
     * @param defaultValue Default value if not found
     * @return Configuration value as float
     */
    float GetFloat(const std::string& path, float defaultValue = 0.0f) const;
    
    /**
     * @brief Get boolean configuration value by path
     * @param path Dot-notation path to value
     * @param defaultValue Default value if not found
     * @return Configuration value as boolean
     */
    bool GetBool(const std::string& path, bool defaultValue = false) const;
    
    /**
     * @brief Get float array configuration value by path
     * @param path Dot-notation path to array
     * @param output Output array to fill
     * @param count Number of elements to extract
     */
    void GetFloatArray(const std::string& path, float* output, size_t count) const;
    
    // === Multi-Type System Accessors ===
    
    /**
     * @brief Get number of defined particle types
     * @return Count of particle types in configuration
     */
    size_t GetParticleTypeCount() const { return simulationConfig.particleTypes.size(); }
    
    /**
     * @brief Get specific particle type by index
     * @param index Index of particle type to retrieve
     * @return Reference to ParticleType structure
     */
    const ParticleType& GetParticleType(size_t index) const { return simulationConfig.particleTypes[index]; }
    
    /**
     * @brief Get all particle type definitions
     * @return Vector of all ParticleType structures
     */
    const std::vector<ParticleType>& GetParticleTypes() const { return simulationConfig.particleTypes; }
    
    /**
     * @brief Get number of defined interaction rules
     * @return Count of interaction rules in configuration
     */
    size_t GetInteractionRuleCount() const { return simulationConfig.interactionRules.size(); }
    
    /**
     * @brief Get specific interaction rule by index
     * @param index Index of interaction rule to retrieve
     * @return Reference to InteractionRule structure
     */
    const InteractionRule& GetInteractionRule(size_t index) const { return simulationConfig.interactionRules[index]; }
    
    /**
     * @brief Get all interaction rule definitions
     * @return Vector of all InteractionRule structures
     */
    const std::vector<InteractionRule>& GetInteractionRules() const { return simulationConfig.interactionRules; }
    
    /**
     * @brief Get particle type distribution settings
     * @return Vector of TypeDistribution structures
     */
    const std::vector<SimulationConfig::TypeDistribution>& GetTypeDistribution() const { return simulationConfig.typeDistribution; }
    
    // === Specialized Configuration Accessors ===
    
    /**
     * @brief Get environmental forces configuration
     * @return Vector of EnvironmentalForce structures
     */
    const std::vector<EnvironmentalForce>& GetEnvironmentalForces() const { return simulationConfig.environmentalForces; }
    
    /**
     * @brief Get culling optimization configuration
     * @return Reference to CullingConfig structure
     */
    const CullingConfig& GetCullingConfig() const { return renderConfig.culling; }
    
    /**
     * @brief Get icosphere rendering configuration
     * @return Reference to IcosphereConfig structure
     */
    const IcosphereConfig& GetIcosphereConfig() const { return renderConfig.icosphere; }
};

// === Template Specializations ===
// Explicit specializations for common type conversions

/**
 * @brief Template specialization for integer parsing
 * @param value String value to convert to integer
 * @return Parsed integer value
 */
template<>
int ConfigManager::ParseValue<int>(const std::string& value) const;

/**
 * @brief Template specialization for float parsing
 * @param value String value to convert to float
 * @return Parsed float value
 */
template<>
float ConfigManager::ParseValue<float>(const std::string& value) const;

/**
 * @brief Template specialization for boolean parsing
 * @param value String value to convert to boolean
 * @return Parsed boolean value
 */
template<>
bool ConfigManager::ParseValue<bool>(const std::string& value) const;

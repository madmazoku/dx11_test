#include "ConfigManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <regex>

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
        ParseJsonAdvanced(content);
        LOG_INFO("Multi-type configuration loaded from: {}", filename);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config file {}: {}", filename, e.what());
        return false;
    }
}

void ConfigManager::ParseJsonAdvanced(const std::string& content) {
    // Advanced JSON parsing for multi-type particle system
    // This is a simplified parser - for production use nlohmann/json
    
    size_t pos = 0;
    
    // Parse simulation settings
    ParseSimulationSettings(content);
    
    // Parse environment settings
    ParseEnvironmentSettings(content);
    
    // Parse particle types
    ParseParticleTypes(content);
    
    // Parse interaction rules
    ParseInteractionRules(content);
    
    // Parse type distribution
    ParseTypeDistribution(content);
    
    // Parse rendering settings
    ParseRenderingSettings(content);
    
    LOG_INFO("Parsed {} particle types, {} interaction rules", 
             simulationConfig.particleTypes.size(), 
             simulationConfig.interactionRules.size());
}

void ConfigManager::ParseSimulationSettings(const std::string& content) {
    // Extract simulation block
    std::regex simRegex(R"("simulation"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    
    if (std::regex_search(content, match, simRegex)) {
        std::string simBlock = match[1].str();
        
        simulationConfig.particleCount = ExtractIntValue(simBlock, "particleCount", 256);
        simulationConfig.timeStep = ExtractFloatValue(simBlock, "timeStep", 0.016f);
        simulationConfig.maxFrames = ExtractIntValue(simBlock, "maxFrames", 0);
        simulationConfig.enableDebugOutput = ExtractBoolValue(simBlock, "enableDebugOutput", true);
        simulationConfig.debugOutputInterval = ExtractIntValue(simBlock, "debugOutputInterval", 60);
        simulationConfig.maxInteractionsPerParticle = ExtractIntValue(simBlock, "maxInteractionsPerParticle", 50);
        simulationConfig.spatialGridSize = ExtractFloatValue(simBlock, "spatialGridSize", 2.0f);
        simulationConfig.enableSpatialOptimization = ExtractBoolValue(simBlock, "enableSpatialOptimization", true);
        simulationConfig.enableMultiThreading = ExtractBoolValue(simBlock, "enableMultiThreading", true);
    }
}

void ConfigManager::ParseEnvironmentSettings(const std::string& content) {
    // Extract environment block
    std::regex envRegex(R"("environment"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    
    if (std::regex_search(content, match, envRegex)) {
        std::string envBlock = match[1].str();
        
        auto gravity = ExtractFloat3Value(envBlock, "gravity", {0.0f, -9.81f, 0.0f});
        simulationConfig.environment.gravity = {gravity[0], gravity[1], gravity[2]};
        
        simulationConfig.environment.airDensity = ExtractFloatValue(envBlock, "airDensity", 1.225f);
        simulationConfig.environment.fluidViscosity = ExtractFloatValue(envBlock, "fluidViscosity", 0.0f);
        simulationConfig.environment.ambientTemperature = ExtractFloatValue(envBlock, "ambientTemperature", 20.0f);
        simulationConfig.environment.thermalDiffusion = ExtractFloatValue(envBlock, "thermalDiffusion", 0.1f);
        
        auto windVel = ExtractFloat3Value(envBlock, "windVelocity", {0.0f, 0.0f, 0.0f});
        simulationConfig.environment.windVelocity = {windVel[0], windVel[1], windVel[2]};
        
        auto boundMin = ExtractFloat3Value(envBlock, "boundaryMin", {-10.0f, -10.0f, -10.0f});
        simulationConfig.environment.boundaryMin = {boundMin[0], boundMin[1], boundMin[2]};
        
        auto boundMax = ExtractFloat3Value(envBlock, "boundaryMax", {10.0f, 10.0f, 10.0f});
        simulationConfig.environment.boundaryMax = {boundMax[0], boundMax[1], boundMax[2]};
        
        simulationConfig.environment.boundaryRestitution = ExtractFloatValue(envBlock, "boundaryRestitution", 0.8f);
        simulationConfig.environment.enableBoundaryHeat = ExtractBoolValue(envBlock, "enableBoundaryHeat", false);
        simulationConfig.environment.boundaryTemperature = ExtractFloatValue(envBlock, "boundaryTemperature", 20.0f);
    }
}

void ConfigManager::ParseParticleTypes(const std::string& content) {
    // Extract particle types array
    std::regex typesRegex(R"("particleTypes"\s*:\s*\[([^\]]*)\])");
    std::smatch match;
    
    if (std::regex_search(content, match, typesRegex)) {
        std::string typesBlock = match[1].str();
        
        // Split by objects
        std::regex objRegex(R"(\{([^}]*)\})");
        std::sregex_iterator iter(typesBlock.begin(), typesBlock.end(), objRegex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string typeObj = (*iter)[1].str();
            ParticleTypeInfo type = ParseParticleType(typeObj);
            simulationConfig.particleTypes.push_back(type);
        }
    }
}

ParticleTypeInfo ConfigManager::ParseParticleType(const std::string& typeObj) {
    ParticleTypeInfo type;
    
    type.id = ExtractIntValue(typeObj, "id", 0);
    type.name = ExtractStringValue(typeObj, "name", "Default");
    type.mass = ExtractFloatValue(typeObj, "mass", 1.0f);
    type.radius = ExtractFloatValue(typeObj, "radius", 0.1f);
    type.density = ExtractFloatValue(typeObj, "density", 1.0f);
    type.charge = ExtractFloatValue(typeObj, "charge", 0.0f);
    type.temperature = ExtractFloatValue(typeObj, "temperature", 20.0f);
    type.restitution = ExtractFloatValue(typeObj, "restitution", 0.8f);
    type.staticFriction = ExtractFloatValue(typeObj, "staticFriction", 0.3f);
    type.dynamicFriction = ExtractFloatValue(typeObj, "dynamicFriction", 0.2f);
    type.enableCollisions = ExtractBoolValue(typeObj, "enableCollisions", true);
    type.airResistance = ExtractFloatValue(typeObj, "airResistance", 0.01f);
    type.thermalConductivity = ExtractFloatValue(typeObj, "thermalConductivity", 1.0f);
    type.lodLevels = ExtractIntValue(typeObj, "lodLevels", 3);
    
    // Parse material
    std::regex materialRegex(R"("material"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    if (std::regex_search(typeObj, match, materialRegex)) {
        std::string materialObj = match[1].str();
        type.material = ParseMaterial(materialObj);
    }
    
    return type;
}

MaterialProperties ConfigManager::ParseMaterial(const std::string& materialObj) {
    MaterialProperties material;
    
    auto diffuse = ExtractFloat3Value(materialObj, "diffuseColor", {1.0f, 1.0f, 1.0f});
    material.diffuseColor = {diffuse[0], diffuse[1], diffuse[2]};
    
    auto specular = ExtractFloat3Value(materialObj, "specularColor", {1.0f, 1.0f, 1.0f});
    material.specularColor = {specular[0], specular[1], specular[2]};
    
    material.shininess = ExtractFloatValue(materialObj, "shininess", 32.0f);
    material.metallic = ExtractFloatValue(materialObj, "metallic", 0.0f);
    material.roughness = ExtractFloatValue(materialObj, "roughness", 0.5f);
    material.reflectance = ExtractFloatValue(materialObj, "reflectance", 0.04f);
    material.emissive = ExtractFloatValue(materialObj, "emissive", 0.0f);
    
    auto emissiveCol = ExtractFloat3Value(materialObj, "emissiveColor", {0.0f, 0.0f, 0.0f});
    material.emissiveColor = {emissiveCol[0], emissiveCol[1], emissiveCol[2]};
    
    return material;
}

void ConfigManager::ParseInteractionRules(const std::string& content) {
    // Extract interaction rules array
    std::regex rulesRegex(R"("interactionRules"\s*:\s*\[([^\]]*)\])");
    std::smatch match;
    
    if (std::regex_search(content, match, rulesRegex)) {
        std::string rulesBlock = match[1].str();
        
        // Split by rule objects (this is complex, simplified version)
        std::regex objRegex(R"(\{(?:[^{}]*|\{[^}]*\})*\})");
        std::sregex_iterator iter(rulesBlock.begin(), rulesBlock.end(), objRegex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string ruleObj = iter->str();
            InteractionRule rule = ParseInteractionRule(ruleObj);
            simulationConfig.interactionRules.push_back(rule);
        }
    }
}

InteractionRule ConfigManager::ParseInteractionRule(const std::string& ruleObj) {
    InteractionRule rule;
    
    rule.typeA = ExtractIntValue(ruleObj, "typeA", 0);
    rule.typeB = ExtractIntValue(ruleObj, "typeB", 0);
    rule.enabled = ExtractBoolValue(ruleObj, "enabled", true);
    rule.temperatureInfluence = ExtractFloatValue(ruleObj, "temperatureInfluence", 0.0f);
    rule.velocityInfluence = ExtractFloatValue(ruleObj, "velocityInfluence", 0.0f);
    
    // Parse force parameters
    std::regex forceRegex(R"("force"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    if (std::regex_search(ruleObj, match, forceRegex)) {
        std::string forceObj = match[1].str();
        rule.force = ParseForceParameters(forceObj);
    }
    
    return rule;
}

ForceParameters ConfigManager::ParseForceParameters(const std::string& forceObj) {
    ForceParameters force;
    
    std::string forceType = ExtractStringValue(forceObj, "type", "None");
    
    // Map string to enum
    if (forceType == "Spring") force.type = ForceType::Spring;
    else if (forceType == "LennardJones") force.type = ForceType::LennardJones;
    else if (forceType == "Gravitational") force.type = ForceType::Gravitational;
    else if (forceType == "Electromagnetic") force.type = ForceType::Electromagnetic;
    else if (forceType == "Viscous") force.type = ForceType::Viscous;
    else force.type = ForceType::None;
    
    force.strength = ExtractFloatValue(forceObj, "strength", 1.0f);
    force.range = ExtractFloatValue(forceObj, "range", 2.0f);
    force.optimalDistance = ExtractFloatValue(forceObj, "optimalDistance", 1.0f);
    
    // Parse type-specific parameters
    switch (force.type) {
        case ForceType::Spring:
            force.spring.stiffness = ExtractFloatValue(forceObj, "stiffness", 1.0f);
            force.spring.dampingCoeff = ExtractFloatValue(forceObj, "dampingCoeff", 0.1f);
            break;
        case ForceType::LennardJones:
            force.lennardJones.epsilon = ExtractFloatValue(forceObj, "epsilon", 1.0f);
            force.lennardJones.sigma = ExtractFloatValue(forceObj, "sigma", 1.0f);
            break;
        case ForceType::Gravitational:
            force.gravitational.gravitationalConstant = ExtractFloatValue(forceObj, "gravitationalConstant", 6.67e-11f);
            break;
        case ForceType::Electromagnetic:
            force.electromagnetic.coulombConstant = ExtractFloatValue(forceObj, "coulombConstant", 8.99e9f);
            break;
        case ForceType::Viscous:
            force.viscous.viscosity = ExtractFloatValue(forceObj, "viscosity", 0.001f);
            force.viscous.dragCoefficient = ExtractFloatValue(forceObj, "dragCoefficient", 0.47f);
            break;
    }
    
    return force;
}

void ConfigManager::ParseTypeDistribution(const std::string& content) {
    // Extract type distribution array
    std::regex distRegex(R"("typeDistribution"\s*:\s*\[([^\]]*)\])");
    std::smatch match;
    
    if (std::regex_search(content, match, distRegex)) {
        std::string distBlock = match[1].str();
        
        std::regex objRegex(R"(\{([^}]*)\})");
        std::sregex_iterator iter(distBlock.begin(), distBlock.end(), objRegex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string distObj = (*iter)[1].str();
            SimulationConfig::TypeDistribution dist;
            
            dist.typeId = ExtractIntValue(distObj, "typeId", 0);
            dist.percentage = ExtractFloatValue(distObj, "percentage", 1.0f);
            
            auto center = ExtractFloat3Value(distObj, "spawnCenter", {0.0f, 0.0f, 0.0f});
            dist.spawnCenter = {center[0], center[1], center[2]};
            
            dist.spawnRadius = ExtractFloatValue(distObj, "spawnRadius", 1.0f);
            
            auto vel = ExtractFloat3Value(distObj, "initialVelocity", {0.0f, 0.0f, 0.0f});
            dist.initialVelocity = {vel[0], vel[1], vel[2]};
            
            simulationConfig.typeDistribution.push_back(dist);
        }
    }
}

void ConfigManager::ParseRenderingSettings(const std::string& content) {
    std::regex renderRegex(R"("rendering"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    
    if (std::regex_search(content, match, renderRegex)) {
        std::string renderBlock = match[1].str();
        
        renderConfig.windowWidth = ExtractIntValue(renderBlock, "windowWidth", 1440);
        renderConfig.windowHeight = ExtractIntValue(renderBlock, "windowHeight", 900);
        renderConfig.enableVSync = ExtractBoolValue(renderBlock, "enableVSync", true);
        
        auto clearCol = ExtractFloat4Value(renderBlock, "clearColor", {0.05f, 0.05f, 0.15f, 1.0f});
        for (int i = 0; i < 4; i++) renderConfig.clearColor[i] = clearCol[i];
        
        // Parse camera, lighting, culling, and icosphere settings
        ParseCameraSettings(renderBlock);
        ParseLightingSettings(renderBlock);
        ParseCullingSettings(renderBlock);
        ParseIcosphereSettings(renderBlock);
    }
}

void ConfigManager::ParseCameraSettings(const std::string& renderBlock) {
    std::regex cameraRegex(R"("camera"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    
    if (std::regex_search(renderBlock, match, cameraRegex)) {
        std::string cameraBlock = match[1].str();
        
        renderConfig.cameraRadius = ExtractFloatValue(cameraBlock, "radius", 15.0f);
        renderConfig.cameraHeight = ExtractFloatValue(cameraBlock, "height", 3.0f);
        renderConfig.cameraRotationSpeed = ExtractFloatValue(cameraBlock, "rotationSpeed", 0.8f);
        renderConfig.cameraFov = ExtractFloatValue(cameraBlock, "fov", 50.0f);
        renderConfig.cameraNearPlane = ExtractFloatValue(cameraBlock, "nearPlane", 0.1f);
        renderConfig.cameraFarPlane = ExtractFloatValue(cameraBlock, "farPlane", 200.0f);
        renderConfig.enableInteractiveCamera = ExtractBoolValue(cameraBlock, "enableInteractive", true);
        renderConfig.cameraZoomMin = ExtractFloatValue(cameraBlock, "zoomMin", 0.05f);
        renderConfig.cameraZoomMax = ExtractFloatValue(cameraBlock, "zoomMax", 1.0f);
        renderConfig.cameraZoomStep = ExtractFloatValue(cameraBlock, "zoomStep", 0.03f);
        renderConfig.cameraRotationSensitivity = ExtractFloatValue(cameraBlock, "rotationSensitivity", 0.008f);
        renderConfig.enableAutoCentering = ExtractBoolValue(cameraBlock, "enableAutoCentering", true);
    }
}

void ConfigManager::ParseLightingSettings(const std::string& renderBlock) {
    std::regex lightRegex(R"("lighting"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    
    if (std::regex_search(renderBlock, match, lightRegex)) {
        std::string lightBlock = match[1].str();
        
        auto dir = ExtractFloat3Value(lightBlock, "direction", {-0.5f, -0.8f, -0.6f});
        for (int i = 0; i < 3; i++) renderConfig.lightDirection[i] = dir[i];
        
        renderConfig.lightIntensity = ExtractFloatValue(lightBlock, "intensity", 1.2f);
        
        auto col = ExtractFloat3Value(lightBlock, "color", {1.0f, 0.95f, 0.9f});
        for (int i = 0; i < 3; i++) renderConfig.lightColor[i] = col[i];
        
        renderConfig.ambientIntensity = ExtractFloatValue(lightBlock, "ambientIntensity", 0.3f);
        renderConfig.specularPower = ExtractFloatValue(lightBlock, "specularPower", 64.0f);
    }
}

void ConfigManager::ParseCullingSettings(const std::string& renderBlock) {
    std::regex cullRegex(R"("culling"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    
    if (std::regex_search(renderBlock, match, cullRegex)) {
        std::string cullBlock = match[1].str();
        
        renderConfig.culling.enableFrustumCulling = ExtractBoolValue(cullBlock, "enableFrustumCulling", true);
        renderConfig.culling.maxRenderDistance = ExtractFloatValue(cullBlock, "maxRenderDistance", 100.0f);
        renderConfig.culling.lodDistance1 = ExtractFloatValue(cullBlock, "lodDistance1", 10.0f);
        renderConfig.culling.lodDistance2 = ExtractFloatValue(cullBlock, "lodDistance2", 25.0f);
        renderConfig.culling.lodDistance3 = ExtractFloatValue(cullBlock, "lodDistance3", 50.0f);
        renderConfig.culling.enableLOD = ExtractBoolValue(cullBlock, "enableLOD", true);
        renderConfig.culling.enableBackfaceCulling = ExtractBoolValue(cullBlock, "enableBackfaceCulling", true);
        renderConfig.culling.enableDistanceCulling = ExtractBoolValue(cullBlock, "enableDistanceCulling", true);
        renderConfig.culling.globalScale = ExtractFloatValue(cullBlock, "globalScale", 1.0f);
        renderConfig.culling.minParticleRadius = ExtractFloatValue(cullBlock, "minParticleRadius", 0.05f);
        renderConfig.culling.maxParticleRadius = ExtractFloatValue(cullBlock, "maxParticleRadius", 0.2f);
    }
}

void ConfigManager::ParseIcosphereSettings(const std::string& renderBlock) {
    std::regex icoRegex(R"("icosphere"\s*:\s*\{([^}]*)\})");
    std::smatch match;
    
    if (std::regex_search(renderBlock, match, icoRegex)) {
        std::string icoBlock = match[1].str();
        
        renderConfig.icosphere.maxSubdivisions = ExtractIntValue(icoBlock, "maxSubdivisions", 3);
        renderConfig.icosphere.enableAdaptiveLOD = ExtractBoolValue(icoBlock, "enableAdaptiveLOD", true);
        renderConfig.icosphere.lodDistance0 = ExtractFloatValue(icoBlock, "lodDistance0", 10.0f);
        renderConfig.icosphere.lodDistance1 = ExtractFloatValue(icoBlock, "lodDistance1", 25.0f);
        renderConfig.icosphere.lodDistance2 = ExtractFloatValue(icoBlock, "lodDistance2", 50.0f);
        renderConfig.icosphere.enableFlatShading = ExtractBoolValue(icoBlock, "enableFlatShading", false);
        renderConfig.icosphere.enableWireframe = ExtractBoolValue(icoBlock, "enableWireframe", false);
    }
}

// Utility functions for parsing values
float ConfigManager::ExtractFloatValue(const std::string& block, const std::string& key, float defaultValue) {
    std::regex regex("\"" + key + "\"\\s*:\\s*([0-9.-]+)");
    std::smatch match;
    if (std::regex_search(block, match, regex)) {
        return std::stof(match[1].str());
    }
    return defaultValue;
}

int ConfigManager::ExtractIntValue(const std::string& block, const std::string& key, int defaultValue) {
    std::regex regex("\"" + key + "\"\\s*:\\s*([0-9-]+)");
    std::smatch match;
    if (std::regex_search(block, match, regex)) {
        return std::stoi(match[1].str());
    }
    return defaultValue;
}

bool ConfigManager::ExtractBoolValue(const std::string& block, const std::string& key, bool defaultValue) {
    std::regex regex("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(block, match, regex)) {
        return match[1].str() == "true";
    }
    return defaultValue;
}

std::string ConfigManager::ExtractStringValue(const std::string& block, const std::string& key, const std::string& defaultValue) {
    std::regex regex("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (std::regex_search(block, match, regex)) {
        return match[1].str();
    }
    return defaultValue;
}

std::vector<float> ConfigManager::ExtractFloat3Value(const std::string& block, const std::string& key, const std::vector<float>& defaultValue) {
    std::regex regex("\"" + key + "\"\\s*:\\s*\\[\\s*([0-9.-]+)\\s*,\\s*([0-9.-]+)\\s*,\\s*([0-9.-]+)\\s*\\]");
    std::smatch match;
    if (std::regex_search(block, match, regex)) {
        return {std::stof(match[1].str()), std::stof(match[2].str()), std::stof(match[3].str())};
    }
    return defaultValue;
}

std::vector<float> ConfigManager::ExtractFloat4Value(const std::string& block, const std::string& key, const std::vector<float>& defaultValue) {
    std::regex regex("\"" + key + "\"\\s*:\\s*\\[\\s*([0-9.-]+)\\s*,\\s*([0-9.-]+)\\s*,\\s*([0-9.-]+)\\s*,\\s*([0-9.-]+)\\s*\\]");
    std::smatch match;
    if (std::regex_search(block, match, regex)) {
        return {std::stof(match[1].str()), std::stof(match[2].str()), std::stof(match[3].str()), std::stof(match[4].str())};
    }
    return defaultValue;
}

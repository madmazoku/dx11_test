#include "PresetManager.h"
#include "Logger.h"

PresetManager::PresetManager() {
    InitializeDefaultPresets();
}

void PresetManager::InitializeDefaultPresets() {
    // Default preset - balanced performance and visuals
    SimulationPreset defaultPreset;
    defaultPreset.name = "default";
    defaultPreset.description = "Balanced simulation with 256 particles";
    
    defaultPreset.simulation.particleCount = 256;
    defaultPreset.simulation.timeStep = 0.016f;
    defaultPreset.simulation.gravity = {0.0f, -9.81f, 0.0f};
    defaultPreset.simulation.damping = 0.995f;
    defaultPreset.simulation.springConstant = 100.0f;
    defaultPreset.simulation.restLength = 0.3f;
    defaultPreset.simulation.boundaryMin = {-8.0f, -8.0f, -8.0f};
    defaultPreset.simulation.boundaryMax = {8.0f, 8.0f, 8.0f};
    defaultPreset.simulation.maxFrames = 0;
    defaultPreset.simulation.enableDebugOutput = true;
    defaultPreset.simulation.debugOutputInterval = 60;
    
    defaultPreset.rendering.windowWidth = 1440;
    defaultPreset.rendering.windowHeight = 900;
    defaultPreset.rendering.enableVSync = true;
    defaultPreset.rendering.clearColor = {0.05f, 0.05f, 0.15f, 1.0f};
    defaultPreset.rendering.sphereRadius = 0.08f;
    defaultPreset.rendering.cameraRadius = 15.0f;
    
    presets[defaultPreset.name] = defaultPreset;
    
    // High-performance preset - fewer particles for smooth 60fps
    SimulationPreset performancePreset;
    performancePreset.name = "performance";
    performancePreset.description = "Optimized for high FPS with 128 particles";
    
    performancePreset.simulation = defaultPreset.simulation;
    performancePreset.simulation.particleCount = 128;
    performancePreset.simulation.springConstant = 80.0f;
    
    performancePreset.rendering = defaultPreset.rendering;
    performancePreset.rendering.sphereRadius = 0.1f;
    
    presets[performancePreset.name] = performancePreset;
    
    // Spectacular preset - many particles for visual impact
    SimulationPreset spectacularPreset;
    spectacularPreset.name = "spectacular";
    spectacularPreset.description = "High particle count for impressive visuals (512 particles)";
    
    spectacularPreset.simulation = defaultPreset.simulation;
    spectacularPreset.simulation.particleCount = 512;
    spectacularPreset.simulation.springConstant = 120.0f;
    spectacularPreset.simulation.damping = 0.998f;
    spectacularPreset.simulation.boundaryMin = {-10.0f, -10.0f, -10.0f};
    spectacularPreset.simulation.boundaryMax = {10.0f, 10.0f, 10.0f};
    
    spectacularPreset.rendering = defaultPreset.rendering;
    spectacularPreset.rendering.sphereRadius = 0.06f;
    spectacularPreset.rendering.cameraRadius = 20.0f;
    
    presets[spectacularPreset.name] = spectacularPreset;
    
    // Zero gravity preset - particles floating in space
    SimulationPreset zeroGPreset;
    zeroGPreset.name = "zerog";
    zeroGPreset.description = "Zero gravity simulation with floating particles";
    
    zeroGPreset.simulation = defaultPreset.simulation;
    zeroGPreset.simulation.gravity = {0.0f, 0.0f, 0.0f};
    zeroGPreset.simulation.damping = 0.999f;
    zeroGPreset.simulation.springConstant = 150.0f;
    
    zeroGPreset.rendering = defaultPreset.rendering;
    zeroGPreset.rendering.clearColor = {0.0f, 0.0f, 0.05f, 1.0f}; // Darker space-like background
    
    presets[zeroGPreset.name] = zeroGPreset;
    
    // Bouncy preset - high energy, lots of bouncing
    SimulationPreset bouncyPreset;
    bouncyPreset.name = "bouncy";
    bouncyPreset.description = "High energy simulation with lots of bouncing";
    
    bouncyPreset.simulation = defaultPreset.simulation;
    bouncyPreset.simulation.damping = 0.98f;
    bouncyPreset.simulation.springConstant = 200.0f;
    bouncyPreset.simulation.gravity = {0.0f, -15.0f, 0.0f};
    bouncyPreset.simulation.boundaryMin = {-6.0f, -6.0f, -6.0f};
    bouncyPreset.simulation.boundaryMax = {6.0f, 6.0f, 6.0f};
    
    bouncyPreset.rendering = defaultPreset.rendering;
    bouncyPreset.rendering.clearColor = {0.1f, 0.05f, 0.05f, 1.0f}; // Slightly reddish
    
    presets[bouncyPreset.name] = bouncyPreset;
}

bool PresetManager::LoadPreset(const std::string& name, SimulationConfig& simConfig, RenderConfig& renderConfig) {
    auto it = presets.find(name);
    if (it == presets.end()) {
        LOG_WARNING("Preset '{}' not found", name);
        return false;
    }
    
    simConfig = it->second.simulation;
    renderConfig = it->second.rendering;
    currentPresetName = name;
    
    LOG_INFO("Loaded preset '{}': {}", name, it->second.description);
    return true;
}

bool PresetManager::SavePreset(const std::string& name, const std::string& description,
                              const SimulationConfig& simConfig, const RenderConfig& renderConfig) {
    SimulationPreset preset;
    preset.name = name;
    preset.description = description;
    preset.simulation = simConfig;
    preset.rendering = renderConfig;
    
    presets[name] = preset;
    LOG_INFO("Saved preset '{}': {}", name, description);
    return true;
}

std::vector<std::string> PresetManager::GetPresetNames() const {
    std::vector<std::string> names;
    for (const auto& pair : presets) {
        names.push_back(pair.first);
    }
    return names;
}

std::string PresetManager::GetPresetDescription(const std::string& name) const {
    auto it = presets.find(name);
    return (it != presets.end()) ? it->second.description : "";
}

bool PresetManager::LoadNextPreset(SimulationConfig& simConfig, RenderConfig& renderConfig) {
    auto names = GetPresetNames();
    if (names.empty()) return false;
    
    auto it = std::find(names.begin(), names.end(), currentPresetName);
    if (it == names.end()) {
        // Current preset not found, load first
        return LoadPreset(names[0], simConfig, renderConfig);
    }
    
    ++it;
    if (it == names.end()) {
        it = names.begin(); // Wrap around
    }
    
    return LoadPreset(*it, simConfig, renderConfig);
}

bool PresetManager::LoadPreviousPreset(SimulationConfig& simConfig, RenderConfig& renderConfig) {
    auto names = GetPresetNames();
    if (names.empty()) return false;
    
    auto it = std::find(names.begin(), names.end(), currentPresetName);
    if (it == names.end()) {
        // Current preset not found, load last
        return LoadPreset(names.back(), simConfig, renderConfig);
    }
    
    if (it == names.begin()) {
        it = names.end(); // Wrap around to end
    }
    --it;
    
    return LoadPreset(*it, simConfig, renderConfig);
}

void PresetManager::PrintAvailablePresets() const {
    LOG_INFO("Available presets:");
    for (const auto& pair : presets) {
        const char* marker = (pair.first == currentPresetName) ? " [CURRENT]" : "";
        LOG_INFO("  {}: {}{}", pair.first, pair.second.description, marker);
    }
}

/**
 * @file PresetManager.cpp
 * @brief Implementation of the PresetManager class for managing simulation configuration presets
 * 
 * The PresetManager provides a convenient way to store, load, and switch between different
 * simulation configurations. It includes several built-in presets optimized for different
 * scenarios (performance, visual quality, special effects) and supports runtime preset
 * switching with keyboard navigation.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#include "PresetManager.h"
#include "Logger.h"

/**
 * @brief Constructs a new PresetManager and initializes default presets
 * 
 * Automatically populates the preset collection with several predefined configurations
 * suitable for different use cases and performance requirements.
 */
PresetManager::PresetManager() {
    InitializeDefaultPresets();
}

/**
 * @brief Initializes the collection of built-in simulation presets
 * 
 * Creates several predefined presets with different characteristics:
 * - Default: Balanced performance and visual quality (256 particles)
 * - Performance: Optimized for high framerate (128 particles)
 * - Spectacular: High particle count for visual impact (512 particles)  
 * - Zero-G: Zero gravity simulation with floating particles
 * - Bouncy: High energy simulation with increased bouncing
 * 
 * Each preset defines complete simulation and rendering parameters including
 * particle counts, physics constants, boundaries, and visual settings.
 */
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

/**
 * @brief Loads a specific preset by name into the provided configuration structures
 * 
 * Searches for the named preset and copies its simulation and rendering parameters
 * into the provided configuration objects. Updates the current preset tracking.
 * 
 * @param name The name of the preset to load
 * @param simConfig Reference to simulation configuration to populate
 * @param renderConfig Reference to render configuration to populate
 * @return true if preset was found and loaded successfully, false otherwise
 */
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

/**
 * @brief Saves current configuration parameters as a new preset
 * 
 * Creates a new preset entry with the specified name and description,
 * storing the current simulation and rendering configurations for later recall.
 * 
 * @param name Unique name for the new preset
 * @param description Human-readable description of the preset
 * @param simConfig Current simulation configuration to save
 * @param renderConfig Current render configuration to save
 * @return true if preset was saved successfully
 */
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

/**
 * @brief Retrieves a list of all available preset names
 * 
 * @return Vector of strings containing all preset names in the collection
 */
std::vector<std::string> PresetManager::GetPresetNames() const {
    std::vector<std::string> names;
    for (const auto& pair : presets) {
        names.push_back(pair.first);
    }
    return names;
}

/**
 * @brief Gets the description for a specific preset
 * 
 * @param name The name of the preset to query
 * @return Description string for the preset, or empty string if not found
 */
std::string PresetManager::GetPresetDescription(const std::string& name) const {
    auto it = presets.find(name);
    return (it != presets.end()) ? it->second.description : "";
}

/**
 * @brief Loads the next preset in the collection (with wrap-around)
 * 
 * Advances to the next preset in alphabetical order, wrapping around to the
 * first preset if currently at the last one. Useful for keyboard navigation.
 * 
 * @param simConfig Reference to simulation configuration to populate
 * @param renderConfig Reference to render configuration to populate
 * @return true if a preset was loaded successfully
 */
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

/**
 * @brief Loads the previous preset in the collection (with wrap-around)
 * 
 * Moves to the previous preset in alphabetical order, wrapping around to the
 * last preset if currently at the first one. Useful for keyboard navigation.
 * 
 * @param simConfig Reference to simulation configuration to populate
 * @param renderConfig Reference to render configuration to populate
 * @return true if a preset was loaded successfully
 */
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

/**
 * @brief Prints all available presets to the log with descriptions
 * 
 * Outputs a formatted list of all presets showing their names, descriptions,
 * and marking the currently active preset for easy identification.
 */
void PresetManager::PrintAvailablePresets() const {
    LOG_INFO("Available presets:");
    for (const auto& pair : presets) {
        const char* marker = (pair.first == currentPresetName) ? " [CURRENT]" : "";
        LOG_INFO("  {}: {}{}", pair.first, pair.second.description, marker);
    }
}

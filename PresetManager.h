#pragma once

#include "Structures.h"
#include <string>
#include <vector>
#include <map>

struct SimulationPreset {
    std::string name;
    std::string description;
    SimulationConfig simulation;
    RenderConfig rendering;
};

class PresetManager {
private:
    std::map<std::string, SimulationPreset> presets;
    std::string currentPresetName = "default";

public:
    PresetManager();
    ~PresetManager() = default;
    
    void InitializeDefaultPresets();
    
    // Preset management
    bool LoadPreset(const std::string& name, SimulationConfig& simConfig, RenderConfig& renderConfig);
    bool SavePreset(const std::string& name, const std::string& description,
                   const SimulationConfig& simConfig, const RenderConfig& renderConfig);
    
    std::vector<std::string> GetPresetNames() const;
    std::string GetCurrentPresetName() const { return currentPresetName; }
    std::string GetPresetDescription(const std::string& name) const;
    
    // Quick access
    bool LoadNextPreset(SimulationConfig& simConfig, RenderConfig& renderConfig);
    bool LoadPreviousPreset(SimulationConfig& simConfig, RenderConfig& renderConfig);
    
    void PrintAvailablePresets() const;
};

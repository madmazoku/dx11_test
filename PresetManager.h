#pragma once

#include "Structures.h"
#include <string>
#include <vector>
#include <map>

/**
 * @file PresetManager.h
 * @brief Preset management system for simulation and rendering configurations
 * 
 * This file provides a comprehensive preset management system that allows users
 * to save, load, and manage different simulation and rendering configurations.
 * The system enables quick switching between different particle simulation
 * setups, making it easy to experiment with various parameters and scenarios.
 * 
 * Key features:
 * - Save and load complete simulation configurations as named presets
 * - Built-in collection of demonstration presets
 * - Runtime preset switching with validation
 * - Preset navigation (next/previous) for easy experimentation
 * - Descriptive metadata for each preset
 * - Thread-safe preset operations
 * 
 * The preset system is essential for rapid prototyping and demonstration
 * of different particle system behaviors and visual effects.
 */

/**
 * @struct SimulationPreset
 * @brief Complete preset containing simulation and rendering configuration
 * 
 * Encapsulates both simulation and rendering parameters along with metadata
 * into a single preset that can be easily saved, loaded, and shared.
 * This structure serves as the storage unit for the preset management system.
 */
struct SimulationPreset {
    std::string name;               ///< Human-readable preset name (unique identifier)
    std::string description;        ///< Detailed description of preset behavior and purpose
    SimulationConfig simulation;    ///< Complete simulation configuration
    RenderConfig rendering;         ///< Complete rendering configuration
};

/**
 * @class PresetManager
 * @brief Management system for simulation and rendering presets
 * 
 * Provides a centralized system for managing simulation presets, enabling users
 * to quickly switch between different particle simulation configurations. The
 * manager maintains a collection of named presets and provides both programmatic
 * and user-friendly interfaces for preset operations.
 * 
 * Preset lifecycle:
 * 1. Initialize with default demonstration presets
 * 2. Allow users to create custom presets from current configuration
 * 3. Provide navigation and loading of existing presets
 * 4. Validate preset integrity on load operations
 * 
 * The system supports both built-in demonstration presets and user-defined
 * configurations, making it suitable for both learning and advanced usage.
 */
class PresetManager {
private:
    std::map<std::string, SimulationPreset> presets;   ///< Storage for all available presets
    std::string currentPresetName = "default";         ///< Currently active preset name

public:
    // === Construction and Lifecycle ===
    
    /**
     * @brief Default constructor
     * 
     * Creates an empty preset manager. Call InitializeDefaultPresets()
     * to populate with built-in demonstration presets.
     */
    PresetManager();
    
    /**
     * @brief Default destructor
     */
    ~PresetManager() = default;
    
    /**
     * @brief Initialize the preset manager with default demonstration presets
     * 
     * Populates the preset manager with a collection of built-in presets
     * that demonstrate various particle system behaviors and effects.
     * These presets serve as starting points and educational examples.
     * 
     * Default presets typically include:
     * - Basic particle fountain
     * - Gravitational system
     * - Electromagnetic interactions
     * - Multi-type force combinations
     * - Performance demonstration scenarios
     */
    void InitializeDefaultPresets();
    
    // === Core Preset Operations ===
    
    /**
     * @brief Load a preset by name
     * 
     * Retrieves the specified preset and copies its configuration to the
     * provided output parameters. This allows the application to apply
     * the preset's settings to the active simulation.
     * 
     * @param name Name of the preset to load
     * @param simConfig Output parameter for simulation configuration
     * @param renderConfig Output parameter for rendering configuration
     * @return true if preset found and loaded successfully, false otherwise
     */
    bool LoadPreset(const std::string& name, SimulationConfig& simConfig, RenderConfig& renderConfig);
    
    /**
     * @brief Save current configuration as a new preset
     * 
     * Creates a new preset from the provided simulation and rendering
     * configurations, allowing users to preserve interesting parameter
     * combinations for future use.
     * 
     * @param name Unique name for the new preset
     * @param description Human-readable description of preset behavior
     * @param simConfig Simulation configuration to save
     * @param renderConfig Rendering configuration to save
     * @return true if preset saved successfully, false if name already exists
     */
    bool SavePreset(const std::string& name, const std::string& description,
                   const SimulationConfig& simConfig, const RenderConfig& renderConfig);
    
    // === Preset Information and Navigation ===
    
    /**
     * @brief Get list of all available preset names
     * 
     * Returns a vector containing the names of all currently available
     * presets, useful for building user interfaces or validation.
     * 
     * @return Vector of preset names in alphabetical order
     */
    std::vector<std::string> GetPresetNames() const;
    
    /**
     * @brief Get the name of the currently active preset
     * @return Name of current preset, or "default" if none set
     */
    std::string GetCurrentPresetName() const { return currentPresetName; }
    
    /**
     * @brief Get description for a specific preset
     * 
     * @param name Name of preset to query
     * @return Description string, or empty string if preset not found
     */
    std::string GetPresetDescription(const std::string& name) const;
    
    // === Quick Navigation ===
    
    /**
     * @brief Load the next preset in alphabetical order
     * 
     * Provides convenient navigation through available presets by loading
     * the next preset alphabetically after the current one. Wraps around
     * to the first preset when reaching the end.
     * 
     * @param simConfig Output parameter for simulation configuration
     * @param renderConfig Output parameter for rendering configuration
     * @return true if next preset loaded successfully, false if no presets available
     */
    bool LoadNextPreset(SimulationConfig& simConfig, RenderConfig& renderConfig);
    
    /**
     * @brief Load the previous preset in alphabetical order
     * 
     * Provides convenient navigation through available presets by loading
     * the previous preset alphabetically before the current one. Wraps around
     * to the last preset when reaching the beginning.
     * 
     * @param simConfig Output parameter for simulation configuration
     * @param renderConfig Output parameter for rendering configuration
     * @return true if previous preset loaded successfully, false if no presets available
     */
    bool LoadPreviousPreset(SimulationConfig& simConfig, RenderConfig& renderConfig);
    
    // === Debug and Information ===
    
    /**
     * @brief Print all available presets to debug output
     * 
     * Outputs a formatted list of all available presets with their names
     * and descriptions to the debug log. Useful for debugging and user
     * interface development.
     */
    void PrintAvailablePresets() const;
};

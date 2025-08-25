/**
 * @file Application.h
 * @brief Main application class header for multi-type particle simulation
 * 
 * This header defines the Application class which serves as the central coordinator
 * for all simulation subsystems. It manages:
 * 
 * - DirectX 11 device initialization and management
 * - Shader loading and compilation (compute, vertex, geometry, pixel)
 * - Multi-type particle system with force-based physics
 * - Interactive camera with mouse controls and auto-centering
 * - Real-time rendering with icosphere geometry and Phong lighting
 * - Statistics overlay and performance monitoring
 * - User input handling and preset management
 * 
 * The Application class implements a standard Windows message loop with
 * real-time physics simulation and rendering at 60+ FPS.
 */

#pragma once

#include <windows.h>
#include <windowsx.h>
#include <memory>
#include <chrono>
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "ParticleSystem.h"
#include "Renderer.h"
#include "InteractiveCamera.h"
#include "ConfigManager.h"
#include "Structures.h"

// Forward declarations to minimize header dependencies
class StatsOverlay;
class PresetManager;

/**
 * Main application class coordinating all simulation subsystems
 * 
 * This class manages the entire application lifecycle from initialization
 * through the main loop to cleanup. It coordinates between DirectX rendering,
 * GPU-based particle physics, user input, and performance monitoring.
 */
class Application {
private:
    // === Core Rendering and Physics Subsystems ===
    std::shared_ptr<D3DDevice> device;                    // DirectX 11 device and swap chain
    std::shared_ptr<ShaderManager> shaderManager;         // HLSL shader loading and management
    std::shared_ptr<ParticleSystem> particleSystem;       // Multi-type particle physics system
    std::shared_ptr<Renderer> renderer;                   // Icosphere-based particle rendering
    std::shared_ptr<InteractiveCamera> interactiveCamera; // User-controlled 3D camera
    std::shared_ptr<ConfigManager> configManager;         // JSON configuration management
    
    // === Optional Enhancement Systems ===
    std::unique_ptr<StatsOverlay> statsOverlay;           // Performance statistics display
    std::unique_ptr<PresetManager> presetManager;         // Simulation preset management
    
    // === Window and Timing Management ===
    HWND hWnd = nullptr;                                   // Main application window handle
    std::chrono::high_resolution_clock::time_point lastFrameTime; // For delta time calculation
    float accumulatedTime = 0.0f;                          // Accumulated time for FPS calculation
    float fps = 0.0f;                                      // Current frames per second
    int frameCount = 0;                                    // Frame counter for FPS calculation
    int totalFrames = 0;                                   // Total frames rendered since start
    
    // === Application State Flags ===
    bool isPaused = false;                                 // Physics simulation pause state
    bool showStatsOverlay = true;                          // Statistics overlay visibility
    
    // === Private Helper Methods ===
    bool CreateWindow();                                   // Initialize Windows window
    bool InitializeSubsystems();                           // Initialize all rendering/physics systems
    bool LoadMultiTypeShaders();                           // Load and compile all HLSL shaders
    void Update();                                         // Update physics and application state
    void Render();                                         // Render frame with particles and UI
    void UpdateWindowTitle();                              // Update window title with statistics
    void HandleKeyInput(unsigned int key);                 // Process keyboard input commands
    
    // === Windows Message Handling ===
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
    // === Constructor and Destructor ===
    Application();                                         // Initialize application instance
    ~Application();                                        // Clean up all resources
    
    // === Main Application Lifecycle Methods ===
    bool Initialize();                                     // Initialize all subsystems and load configuration
    void Run();                                           // Execute main application loop until exit
    void Shutdown();                                      // Clean shutdown of all systems
    
    // === Public Accessors for System Information ===
    HWND GetWindowHandle() const { return hWnd; }         // Get Windows window handle
    float GetFPS() const { return fps; }                  // Get current frame rate
    bool IsPaused() const { return isPaused; }            // Check if simulation is paused
    
    // === Subsystem Access for Integration ===
    std::shared_ptr<ConfigManager> GetConfigManager() const { return configManager; }
    std::shared_ptr<ParticleSystem> GetParticleSystem() const { return particleSystem; }
    std::shared_ptr<Renderer> GetRenderer() const { return renderer; }
};

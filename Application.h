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

// Forward declarations
class StatsOverlay;
class PresetManager;

class Application {
private:
    // Core subsystems
    std::shared_ptr<D3DDevice> device;
    std::shared_ptr<ShaderManager> shaderManager;
    std::shared_ptr<ParticleSystem> particleSystem;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<InteractiveCamera> interactiveCamera;
    std::shared_ptr<ConfigManager> configManager;
    
    // Optional systems
    std::unique_ptr<StatsOverlay> statsOverlay;
    std::unique_ptr<PresetManager> presetManager;
    
    // Window and timing
    HWND hWnd = nullptr;
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    float accumulatedTime = 0.0f;
    float fps = 0.0f;
    int frameCount = 0;
    int totalFrames = 0;
    
    // Application state
    bool isPaused = false;
    bool showStatsOverlay = true;
    
    // Helper methods
    bool CreateWindow();
    bool InitializeSubsystems();
    bool LoadMultiTypeShaders();
    void Update();
    void Render();
    void UpdateWindowTitle();
    void HandleKeyInput(unsigned int key);
    
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
    Application();
    ~Application();
    
    // Application lifecycle
    bool Initialize();
    void Run();
    void Shutdown();
    
    // Accessors
    HWND GetWindowHandle() const { return hWnd; }
    float GetFPS() const { return fps; }
    bool IsPaused() const { return isPaused; }
    
    // Configuration
    std::shared_ptr<ConfigManager> GetConfigManager() const { return configManager; }
    std::shared_ptr<ParticleSystem> GetParticleSystem() const { return particleSystem; }
    std::shared_ptr<Renderer> GetRenderer() const { return renderer; }
};

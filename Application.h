#pragma once

#include "D3DDevice.h"
#include "ShaderManager.h"
#include "ParticleSystem.h" 
#include "Renderer.h"
#include "InteractiveCamera.h"
#include "StatsOverlay.h"
#include "PresetManager.h"
#include "Structures.h"
#include <Windows.h>
#include <memory>
#include <chrono>

class Application {
private:
    HWND hWnd = nullptr;
    
    std::shared_ptr<D3DDevice> device;
    std::shared_ptr<ShaderManager> shaderManager;
    std::shared_ptr<ParticleSystem> particleSystem;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<InteractiveCamera> camera;
    std::shared_ptr<StatsOverlay> statsOverlay;
    std::shared_ptr<PresetManager> presetManager;
    
    SimulationConfig simulationConfig;
    RenderConfig renderConfig;
    
    bool running = true;
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    float totalTime = 0.0f;
    int frameCount = 0;

public:
    Application();
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    bool CreateWindow();
    bool InitializeSubsystems();
    void GameLoop();
    void Update(float deltaTime);
    void Render(float deltaTime);
    void ProcessMessages();
    
    // Window event handling
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    void OnKeyDown(WPARAM key);
    void OnResize(int width, int height);
    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y, bool leftButton, bool rightButton);
    void OnMouseUp(int x, int y, bool leftButton, bool rightButton);
    void OnMouseWheel(int delta);
    
    void LoadConfiguration();
    void ShowDebugInfo();
    void ApplyNewConfiguration();
};

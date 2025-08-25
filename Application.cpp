/**
 * @file Application.cpp
 * @brief Main application class for particle simulation
 * 
 * This file contains the core application logic, including:
 * - Initialization of all subsystems (DirectX, shaders, particle system)
 * - Main game loop with physics updates and rendering
 * - User input handling and camera management
 * - Application lifecycle management
 * 
 * The Application class serves as the central coordinator for all systems,
 * managing their initialization, updates, and cleanup. It implements a
 * standard Windows message loop with real-time rendering and physics simulation.
 */

#include "Application.h"
#include "Utilities.h"
#include "Logger.h"
#include "Profiler.h"
#include "ConfigManager.h"
#include "StatsOverlay.h"
#include "PresetManager.h"
#include <iostream>
#include <filesystem>

namespace {
    // Global pointer to application instance for Windows message handling
    // This allows the static WindowProc function to access the Application instance
    Application* g_pApp = nullptr;
}

Application::Application() {
    // Set global application pointer for Windows message handling
    g_pApp = this;
    
    // Initialize timing for frame rate calculations and delta time
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

Application::~Application() {
    // Clean shutdown of all systems
    Shutdown();
    
    // Clear global application pointer
    g_pApp = nullptr;
}

bool Application::Initialize() {
    // Initialize logging system with info level and console output
    Logger::GetInstance().Initialize("multitype_particle_simulation.log", LogLevel::Info, true);
    LOG_INFO("Multi-type particle simulation initializing...");
    
    // Load configuration from JSON file containing particle types, forces, and rendering settings
    configManager = std::make_shared<ConfigManager>();
    if (!configManager->LoadFromFile("config.json")) {
        LOG_ERROR("Failed to load multi-type configuration");
        return false;
    }
    
    // Enable performance profiling for debugging and optimization
    Profiler::GetInstance().SetEnabled(true);
    
    try {
        // Initialize Windows window and DirectX 11 subsystems
        if (!CreateWindow()) return false;
        if (!InitializeSubsystems()) return false;
        
        LOG_INFO("Multi-type particle simulation initialized successfully");
        LOG_INFO("Configuration: {} particle types, {} interaction rules, {} particles",
                configManager->GetParticleTypeCount(),
                configManager->GetInteractionRuleCount(),
                configManager->GetSimulationConfig().particleCount);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Multi-type application initialization failed: {}", e.what());
        return false;
    }
}

bool Application::CreateWindow() {
    PROFILE_FUNCTION();
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MultiTypeParticleSimulation";

    if (!RegisterClassExW(&wc)) {
        LOG_ERROR("Failed to register window class");
        return false;
    }

    const auto& renderConfig = configManager->GetRenderConfig();
    
    hWnd = CreateWindowExW(
        0,
        L"MultiTypeParticleSimulation", 
        L"Multi-Type Particle Physics - Advanced Forces & Materials",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        renderConfig.windowWidth + 16, renderConfig.windowHeight + 39,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!hWnd) {
        LOG_ERROR("Failed to create window");
        return false;
    }

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);
    
    LOG_INFO("Window created successfully ({}x{})", renderConfig.windowWidth, renderConfig.windowHeight);
    return true;
}

bool Application::InitializeSubsystems() {
    PROFILE_FUNCTION();
    
    // Create DirectX 11 device and initialize with window handle and dimensions
    device = std::make_shared<D3DDevice>();
    if (!device->Initialize(hWnd, configManager->GetRenderConfig().windowWidth, 
                           configManager->GetRenderConfig().windowHeight)) {
        LOG_ERROR("Failed to initialize D3D device");
        return false;
    }

    // Create shader manager for loading and managing HLSL compute, vertex, geometry, and pixel shaders
    shaderManager = std::make_shared<ShaderManager>(device);
    if (!shaderManager->Initialize()) {
        LOG_ERROR("Failed to initialize shader manager");
        return false;
    }

    // Load all required shaders: compute, vertex, geometry, pixel, and frustum culling
    if (!LoadMultiTypeShaders()) {
        LOG_ERROR("Failed to load multi-type shaders");
        return false;
    }

    // Create multi-type particle system with force-based interactions and GPU simulation
    particleSystem = std::make_shared<ParticleSystem>(device, configManager);
    if (!particleSystem->Initialize()) {
        LOG_ERROR("Failed to initialize multi-type particle system");
        return false;
    }

    // Create renderer with icosphere-based particle rendering and Phong shading
    renderer = std::make_shared<Renderer>(device, shaderManager, configManager);
    if (!renderer->Initialize()) {
        LOG_ERROR("Failed to initialize multi-type renderer");
        return false;
    }

    // Create interactive camera system with mouse controls and auto-centering
    interactiveCamera = std::make_shared<InteractiveCamera>(configManager->GetRenderConfig());
    if (!interactiveCamera->Initialize()) {
        LOG_ERROR("Failed to initialize interactive camera");
        return false;
    }

    // Initialize optional statistics overlay for performance monitoring
    statsOverlay = std::make_unique<StatsOverlay>();
    if (!statsOverlay->Initialize()) {
        LOG_WARNING("Failed to initialize stats overlay");
        statsOverlay.reset();
    }

    // Initialize preset manager for simulation configuration switching
    presetManager = std::make_unique<PresetManager>(configManager);
    if (!presetManager->LoadPresets("presets")) {
        LOG_WARNING("Failed to load simulation presets");
    }

    LOG_INFO("All subsystems initialized successfully");
    return true;
}

bool Application::LoadMultiTypeShaders() {
    PROFILE_FUNCTION();
    
    // Load main compute shader for particle physics simulation with multi-type force interactions
    // This shader handles spring forces, Lennard-Jones potentials, electromagnetic forces, etc.
    if (!shaderManager->CreateComputeShaderFromFile("ComputeShader.hlsl", "CSMain", "ComputeShader")) {
        LOG_ERROR("Failed to load multi-type compute shader");
        return false;
    }

    // Load vertex shader that prepares particle data for geometry shader icosphere generation
    // Transforms particle positions and passes type information for material selection
    if (!shaderManager->CreateVertexShaderFromFile("VertexShader.hlsl", "VSMain", "VertexShader")) {
        LOG_ERROR("Failed to load multi-type vertex shader");
        return false;
    }

    // Load geometry shader that generates icospheres from particle points with LOD support
    // Creates detailed 3D spherical geometry based on distance and particle type properties
    if (!shaderManager->CreateGeometryShaderFromFile("GeometryShaderIcosphere.hlsl", "GSMain", "GeometryShaderIcosphere")) {
        LOG_ERROR("Failed to load icosphere geometry shader");
        return false;
    }

    // Load pixel shader implementing Phong lighting model with material-based shading
    // Provides realistic lighting with specular highlights and material property support
    if (!shaderManager->CreatePixelShaderFromFile("PixelShaderPhong.hlsl", "PSMain", "PixelShaderPhong")) {
        LOG_ERROR("Failed to load Phong pixel shader");
        return false;
    }

    // Load optional frustum culling compute shader for performance optimization
    // Performs GPU-based visibility culling to improve rendering performance with large particle counts
    if (!shaderManager->CreateComputeShaderFromFile("FrustumCulling.hlsl", "CSMain", "FrustumCulling")) {
        LOG_WARNING("Failed to load frustum culling shader - culling will be disabled");
    }

    LOG_INFO("Multi-type shaders loaded successfully");
    return true;
}

void Application::Run() {
    LOG_INFO("Starting main application loop");
    
    // Main message loop
    MSG msg = {};
    bool isRunning = true;
    
    while (isRunning && msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            if (msg.message == WM_QUIT) {
                isRunning = false;
            }
        } else {
            Update();
            Render();
        }
    }
    
    LOG_INFO("Application loop ended");
}

void Application::Update() {
    PROFILE_FUNCTION();
    
    // Calculate frame delta time for smooth, frame-rate independent updates
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
    
    // Limit delta time for stability - prevents large time steps that could destabilize physics
    deltaTime = std::min(deltaTime, 0.033f); // Cap at 30fps minimum
    
    // Accumulate timing information for FPS calculation
    frameCount++;
    accumulatedTime += deltaTime;
    
    // Update FPS counter every second for stable measurements
    if (accumulatedTime >= 1.0f) {
        fps = frameCount / accumulatedTime;
        frameCount = 0;
        accumulatedTime = 0.0f;
        
        // Update window title with current performance statistics
        UpdateWindowTitle();
    }
    
    // Update interactive camera system with particle-based auto-centering and user input
    if (interactiveCamera && configManager->GetRenderConfig().enableInteractiveCamera) {
        // Auto-center camera based on particle distribution
        interactiveCamera->UpdateFromParticles(particleSystem->GetParticles());
        // Apply user input (mouse drag, wheel zoom, etc.)
        interactiveCamera->Update(deltaTime);
    }
    
    // Update particle physics simulation on GPU using compute shaders
    // This includes force calculations, position updates, and collision handling
    particleSystem->Update(shaderManager);
    
    // Update statistics overlay with current performance and simulation data
    if (statsOverlay) {
        statsOverlay->Update(deltaTime, fps, particleSystem->GetParticleCount(),
                           particleSystem->GetTypeCount(), particleSystem->GetRuleCount());
    }
    
    // Output debug information at regular intervals for performance monitoring
    const auto& simConfig = configManager->GetSimulationConfig();
    if (simConfig.enableDebugOutput && frameCount % simConfig.debugOutputInterval == 0) {
        LOG_INFO("Frame: {}, FPS: {:.1f}, Particles: {}, Types: {}, Rules: {}", 
                totalFrames, fps, particleSystem->GetParticleCount(),
                particleSystem->GetTypeCount(), particleSystem->GetRuleCount());
    }
    
    totalFrames++;
    
    // Check if we've reached the maximum frame limit (for benchmarking/testing)
    if (simConfig.maxFrames > 0 && totalFrames >= simConfig.maxFrames) {
        LOG_INFO("Reached maximum frames ({}), exiting", simConfig.maxFrames);
        PostQuitMessage(0);
    }
}

void Application::Render() {
    PROFILE_FUNCTION();
    
    // Ensure all required systems are available before rendering
    if (!device || !renderer || !interactiveCamera || !particleSystem) return;
    
    // Get current camera transformation matrices for 3D rendering
    auto viewMatrix = interactiveCamera->GetViewMatrix();        // World-to-view transformation
    auto projMatrix = interactiveCamera->GetProjectionMatrix();  // View-to-screen projection
    auto cameraPos = interactiveCamera->GetPosition();           // Camera world position for lighting
    
    // Update renderer with current particle simulation state for GPU buffers
    renderer->UpdateSimulationConstants(particleSystem->GetParticles());
    
    // Render all visible particles using icosphere geometry with Phong lighting
    // Includes frustum culling, LOD selection, and material-based shading
    renderer->RenderFrameWithCamera(particleSystem->GetParticles(), viewMatrix, projMatrix, cameraPos);
    
    // Render optional statistics overlay on top of 3D scene
    if (statsOverlay && showStatsOverlay) {
        statsOverlay->Render();
    }
    
    // Present the completed frame to screen with optional VSync
    device->Present(configManager->GetRenderConfig().enableVSync);
}

void Application::UpdateWindowTitle() {
    if (!hWnd) return;
    
    std::wstring title = L"Multi-Type Particle Physics - Advanced Forces & Materials | ";
    title += L"FPS: " + std::to_wstring(static_cast<int>(fps));
    title += L" | Particles: " + std::to_wstring(particleSystem->GetParticleCount());
    title += L" | Types: " + std::to_wstring(particleSystem->GetTypeCount());
    title += L" | Rules: " + std::to_wstring(particleSystem->GetRuleCount());
    
    SetWindowTextW(hWnd, title.c_str());
}

void Application::Shutdown() {
    LOG_INFO("Application shutting down...");
    
    // Reset smart pointers in reverse order of creation
    presetManager.reset();
    statsOverlay.reset();
    interactiveCamera.reset();
    renderer.reset();
    particleSystem.reset();
    shaderManager.reset();
    device.reset();
    configManager.reset();
    
    if (hWnd) {
        DestroyWindow(hWnd);
        hWnd = nullptr;
    }
    
    LOG_INFO("Application shutdown complete");
}

LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (g_pApp) {
        return g_pApp->HandleWindowMessage(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT Application::HandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE:
            if (device && wParam != SIZE_MINIMIZED) {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                device->OnWindowResize(width, height);
                
                if (interactiveCamera) {
                    interactiveCamera->OnWindowResize(width, height);
                }
                
                LOG_INFO("Window resized to {}x{}", width, height);
            }
            return 0;
            
        case WM_KEYDOWN:
            HandleKeyInput(static_cast<unsigned int>(wParam));
            return 0;
            
        case WM_LBUTTONDOWN:
            if (interactiveCamera) {
                SetCapture(hWnd);
                interactiveCamera->StartMouseDrag(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;
            
        case WM_LBUTTONUP:
            if (interactiveCamera) {
                ReleaseCapture();
                interactiveCamera->EndMouseDrag();
            }
            return 0;
            
        case WM_MOUSEMOVE:
            if (interactiveCamera && (wParam & MK_LBUTTON)) {
                interactiveCamera->UpdateMouseDrag(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;
            
        case WM_MOUSEWHEEL:
            if (interactiveCamera) {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                interactiveCamera->UpdateMouseWheel(delta);
            }
            return 0;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void Application::HandleKeyInput(unsigned int key) {
    switch (key) {
        case VK_ESCAPE:
            // Exit application immediately
            PostQuitMessage(0);
            break;
            
        case VK_F1:
            // Toggle statistics overlay display for performance monitoring
            showStatsOverlay = !showStatsOverlay;
            LOG_INFO("Stats overlay {}", showStatsOverlay ? "enabled" : "disabled");
            break;
            
        case VK_F2:
            // Toggle GPU-based frustum culling for performance optimization
            if (renderer) {
                renderer->EnableFrustumCulling(!renderer->IsFrustumCullingEnabled());
                LOG_INFO("Frustum culling {}", 
                        renderer->IsFrustumCullingEnabled() ? "enabled" : "disabled");
            }
            break;
            
        case VK_F3:
            // Toggle automatic camera centering based on particle distribution
            if (interactiveCamera) {
                interactiveCamera->EnableAutoCentering(!interactiveCamera->IsAutoCenteringEnabled());
                LOG_INFO("Auto-centering {}", 
                        interactiveCamera->IsAutoCenteringEnabled() ? "enabled" : "disabled");
            }
            break;
            
        case VK_R:
            // Reset entire particle simulation to initial state
            if (particleSystem) {
                LOG_INFO("Resetting simulation...");
                particleSystem.reset();
                particleSystem = std::make_shared<ParticleSystem>(device, configManager);
                particleSystem->Initialize();
            }
            break;
            
        case VK_P:
            // Pause or resume physics simulation (rendering continues)
            isPaused = !isPaused;
            LOG_INFO("Simulation {}", isPaused ? "paused" : "resumed");
            break;
            
        case '1': case '2': case '3': case '4': case '5':
            // Load predefined simulation presets with different particle configurations
            if (presetManager) {
                int presetIndex = key - '1';
                if (presetManager->LoadPreset(presetIndex)) {
                    LOG_INFO("Loaded preset {}", presetIndex + 1);
                    // Reinitialize particle system with new configuration parameters
                    particleSystem.reset();
                    particleSystem = std::make_shared<ParticleSystem>(device, configManager);
                    particleSystem->Initialize();
                }
            }
            break;
    }
}

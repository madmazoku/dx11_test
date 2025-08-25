#include "Application.h"
#include "Utilities.h"
#include "Logger.h"
#include "Profiler.h"
#include "ConfigManager.h"
#include "StatsOverlay.h"
#include "PresetManager.h"
#include <iostream>    if (!shaderManager->CreateComputeShaderFromFile("FrustumCulling.hlsl", "CSMain", "FrustumCulling")) {
        LOG_WARNING("Failed to load frustum culling compute shader - culling will be disabled");
    }nclude <filesystem>

namespace {
    Application* g_pApp = nullptr;
}

Application::Application() {
    g_pApp = this;
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

Application::~Application() {
    Shutdown();
    g_pApp = nullptr;
}

bool Application::Initialize() {
    // Initialize logger
    Logger::GetInstance().Initialize("multitype_particle_simulation.log", LogLevel::Info, true);
    LOG_INFO("Multi-type particle simulation initializing...");
    
    // Load multi-type configuration
    configManager = std::make_shared<ConfigManager>();
    if (!configManager->LoadFromFile("config.json")) {
        LOG_ERROR("Failed to load multi-type configuration");
        return false;
    }
    
    // Initialize profiler
    Profiler::GetInstance().SetEnabled(true);
    
    try {
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
    
    // Create D3D device
    device = std::make_shared<D3DDevice>();
    if (!device->Initialize(hWnd, configManager->GetRenderConfig().windowWidth, 
                           configManager->GetRenderConfig().windowHeight)) {
        LOG_ERROR("Failed to initialize D3D device");
        return false;
    }

    // Create shader manager
    shaderManager = std::make_shared<ShaderManager>(device);
    if (!shaderManager->Initialize()) {
        LOG_ERROR("Failed to initialize shader manager");
        return false;
    }

    // Load multi-type shaders
    if (!LoadMultiTypeShaders()) {
        LOG_ERROR("Failed to load multi-type shaders");
        return false;
    }

    // Create multi-type particle system
    particleSystem = std::make_shared<ParticleSystem>(device, configManager);
    if (!particleSystem->Initialize()) {
        LOG_ERROR("Failed to initialize multi-type particle system");
        return false;
    }

    // Create renderer with multi-type support
    renderer = std::make_shared<Renderer>(device, shaderManager, configManager);
    if (!renderer->Initialize()) {
        LOG_ERROR("Failed to initialize multi-type renderer");
        return false;
    }

    // Create interactive camera
    interactiveCamera = std::make_shared<InteractiveCamera>(configManager->GetRenderConfig());
    if (!interactiveCamera->Initialize()) {
        LOG_ERROR("Failed to initialize interactive camera");
        return false;
    }

    // Initialize stats overlay
    statsOverlay = std::make_unique<StatsOverlay>();
    if (!statsOverlay->Initialize()) {
        LOG_WARNING("Failed to initialize stats overlay");
        statsOverlay.reset();
    }

    // Initialize preset manager
    presetManager = std::make_unique<PresetManager>(configManager);
    if (!presetManager->LoadPresets("presets")) {
        LOG_WARNING("Failed to load simulation presets");
    }

    LOG_INFO("All subsystems initialized successfully");
    return true;
}

bool Application::LoadMultiTypeShaders() {
    PROFILE_FUNCTION();
    
    // Load multi-type compute shader
    if (!shaderManager->CreateComputeShaderFromFile("ComputeShader.hlsl", "CSMain", "ComputeShader")) {
        LOG_ERROR("Failed to load multi-type compute shader");
        return false;
    }

    // Load multi-type vertex shader
    if (!shaderManager->CreateVertexShaderFromFile("VertexShader.hlsl", "VSMain", "VertexShader")) {
        LOG_ERROR("Failed to load multi-type vertex shader");
        return false;
    }

    // Load icosphere geometry shader
    if (!shaderManager->CreateGeometryShaderFromFile("GeometryShaderIcosphere.hlsl", "GSMain", "GeometryShaderIcosphere")) {
        LOG_ERROR("Failed to load icosphere geometry shader");
        return false;
    }

    // Load Phong pixel shader
    if (!shaderManager->CreatePixelShaderFromFile("PixelShaderPhong.hlsl", "PSMain", "PixelShaderPhong")) {
        LOG_ERROR("Failed to load Phong pixel shader");
        return false;
    }

    // Load frustum culling shader
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
    
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
    
    // Limit delta time for stability
    deltaTime = std::min(deltaTime, 0.033f); // Cap at 30fps
    
    frameCount++;
    accumulatedTime += deltaTime;
    
    // Update FPS every second
    if (accumulatedTime >= 1.0f) {
        fps = frameCount / accumulatedTime;
        frameCount = 0;
        accumulatedTime = 0.0f;
        
        // Update window title with stats
        UpdateWindowTitle();
    }
    
    // Update interactive camera
    if (interactiveCamera && configManager->GetRenderConfig().enableInteractiveCamera) {
        interactiveCamera->UpdateFromParticles(particleSystem->GetParticles());
        interactiveCamera->Update(deltaTime);
    }
    
    // Update particle system
    particleSystem->Update(shaderManager);
    
    // Update stats overlay
    if (statsOverlay) {
        statsOverlay->Update(deltaTime, fps, particleSystem->GetParticleCount(),
                           particleSystem->GetTypeCount(), particleSystem->GetRuleCount());
    }
    
    // Debug output
    const auto& simConfig = configManager->GetSimulationConfig();
    if (simConfig.enableDebugOutput && frameCount % simConfig.debugOutputInterval == 0) {
        LOG_INFO("Frame: {}, FPS: {:.1f}, Particles: {}, Types: {}, Rules: {}", 
                totalFrames, fps, particleSystem->GetParticleCount(),
                particleSystem->GetTypeCount(), particleSystem->GetRuleCount());
    }
    
    totalFrames++;
    
    // Check frame limit
    if (simConfig.maxFrames > 0 && totalFrames >= simConfig.maxFrames) {
        LOG_INFO("Reached maximum frames ({}), exiting", simConfig.maxFrames);
        PostQuitMessage(0);
    }
}

void Application::Render() {
    PROFILE_FUNCTION();
    
    if (!device || !renderer || !interactiveCamera || !particleSystem) return;
    
    // Get camera matrices
    auto viewMatrix = interactiveCamera->GetViewMatrix();
    auto projMatrix = interactiveCamera->GetProjectionMatrix();
    auto cameraPos = interactiveCamera->GetPosition();
    
    // Update renderer simulation constants
    renderer->UpdateSimulationConstants(particleSystem->GetParticles());
    
    // Render particles with camera
    renderer->RenderFrameWithCamera(particleSystem->GetParticles(), viewMatrix, projMatrix, cameraPos);
    
    // Render stats overlay
    if (statsOverlay && showStatsOverlay) {
        statsOverlay->Render();
    }
    
    // Present frame
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
            PostQuitMessage(0);
            break;
            
        case VK_F1:
            showStatsOverlay = !showStatsOverlay;
            LOG_INFO("Stats overlay {}", showStatsOverlay ? "enabled" : "disabled");
            break;
            
        case VK_F2:
            if (renderer) {
                renderer->EnableFrustumCulling(!renderer->IsFrustumCullingEnabled());
                LOG_INFO("Frustum culling {}", 
                        renderer->IsFrustumCullingEnabled() ? "enabled" : "disabled");
            }
            break;
            
        case VK_F3:
            if (interactiveCamera) {
                interactiveCamera->EnableAutoCentering(!interactiveCamera->IsAutoCenteringEnabled());
                LOG_INFO("Auto-centering {}", 
                        interactiveCamera->IsAutoCenteringEnabled() ? "enabled" : "disabled");
            }
            break;
            
        case VK_R:
            // Reset simulation
            if (particleSystem) {
                LOG_INFO("Resetting simulation...");
                particleSystem.reset();
                particleSystem = std::make_shared<ParticleSystem>(device, configManager);
                particleSystem->Initialize();
            }
            break;
            
        case VK_P:
            // Pause/unpause simulation
            isPaused = !isPaused;
            LOG_INFO("Simulation {}", isPaused ? "paused" : "resumed");
            break;
            
        case '1': case '2': case '3': case '4': case '5':
            // Load presets
            if (presetManager) {
                int presetIndex = key - '1';
                if (presetManager->LoadPreset(presetIndex)) {
                    LOG_INFO("Loaded preset {}", presetIndex + 1);
                    // Reinitialize systems with new configuration
                    particleSystem.reset();
                    particleSystem = std::make_shared<ParticleSystem>(device, configManager);
                    particleSystem->Initialize();
                }
            }
            break;
    }
}

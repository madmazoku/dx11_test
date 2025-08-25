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
    Logger::GetInstance().Initialize("particle_simulation.log", LogLevel::Info, true);
    LOG_INFO("Application initializing...");
    
    // Load configuration
    ConfigManager configManager;
    if (configManager.LoadFromFile("config.json")) {
        simulationConfig = configManager.GetSimulationConfig();
        renderConfig = configManager.GetRenderConfig();
    } else {
        LOG_WARNING("Using default configuration");
        // Keep default values from struct initialization
    }
    
    // Initialize profiler
    Profiler::GetInstance().SetEnabled(true);
    
    try {
        if (!CreateWindow()) return false;
        if (!InitializeSubsystems()) return false;
        
        LOG_INFO("Application initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Application initialization failed: " << e.what() << std::endl;
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
    wc.lpszClassName = L"ParticleSimulation";

    if (!RegisterClassExW(&wc)) {
        LOG_ERROR("Failed to register window class");
        return false;
    }

    hWnd = CreateWindowExW(
        0,
        L"ParticleSimulation", 
        L"Particle Physics - Verlet Integration",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        renderConfig.windowWidth + 16, renderConfig.windowHeight + 39, // Account for borders
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
    device = std::make_shared<D3DDevice>(renderConfig);
    if (!device->Initialize(hWnd)) {
        LOG_ERROR("Failed to initialize D3D device");
        return false;
    }

    // Create shader manager
    shaderManager = std::make_shared<ShaderManager>(device);
    std::filesystem::path executableDir;
    
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    executableDir = std::filesystem::path(buffer).parent_path();
    
    if (!shaderManager->LoadAllShaders(executableDir)) {
        LOG_ERROR("Failed to load shaders");
        return false;
    }

    // Create particle system
    particleSystem = std::make_shared<ParticleSystem>(device, simulationConfig);
    if (!particleSystem->Initialize()) {
        LOG_ERROR("Failed to initialize particle system");
        return false;
    }

    // Create renderer
    renderer = std::make_shared<Renderer>(device, shaderManager, renderConfig);
    if (!renderer->Initialize()) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
    }

    // Create interactive camera
    camera = std::make_shared<InteractiveCamera>(renderConfig);

    // Create stats overlay
    statsOverlay = std::make_shared<StatsOverlay>();

    // Create preset manager
    presetManager = std::make_shared<PresetManager>();
    presetManager->PrintAvailablePresets();

    LOG_INFO("All subsystems initialized successfully");
    return true;
}

void Application::Run() {
    LOG_INFO("Starting particle simulation with {} particles", simulationConfig.particleCount);
    LOG_INFO("Press ESC to exit, R to reset simulation");
    
    GameLoop();
}

void Application::GameLoop() {
    while (running && (simulationConfig.maxFrames <= 0 || frameCount < simulationConfig.maxFrames)) {
        PROFILE_SCOPE("Frame");
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        ProcessMessages();
        
        if (running) {
            Update(deltaTime);
            Render(deltaTime);
            frameCount++;
            totalTime += deltaTime;
            
            // Performance reporting
            if (simulationConfig.enableDebugOutput && frameCount % simulationConfig.debugOutputInterval == 0) {
                LOG_INFO("Frame {}: FPS={:.1f}, Time={:.3f}s", 
                    frameCount, 1.0f / deltaTime, totalTime);
                    
                if (frameCount % (simulationConfig.debugOutputInterval * 10) == 0) {
                    Profiler::GetInstance().PrintStatistics();
                }
            }
        }
    }
    
    LOG_INFO("Simulation completed after {} frames ({:.2f}s)", frameCount, totalTime);
    Profiler::GetInstance().PrintStatistics();
}
void Application::Update(float deltaTime) {
    PROFILE_SCOPE("Update");
    
    // Update particle simulation
    particleSystem->Update();
    
    // Update camera bounds based on particle positions
    // For now we'll read back particle positions every few frames for performance
    if (frameCount % 10 == 0) { // Update camera bounds every 10 frames
        auto particles = particleSystem->ReadBackParticles();
        camera->UpdateCloudBounds(particles);
    }
    
    // Update stats overlay
    if (statsOverlay) {
        float currentFPS = (deltaTime > 0.0f) ? 1.0f / deltaTime : 0.0f;
        statsOverlay->Update(deltaTime, currentFPS, *particleSystem, *camera, totalTime);
    }
}

void Application::Render(float deltaTime) {
    PROFILE_SCOPE("Render");
    
    device->ClearRenderTarget();
    renderer->RenderFrameWithCamera(*particleSystem, *camera, totalTime);
    device->Present();
}

void Application::ShowDebugInfo() {
    try {
        auto particles = particleSystem->ReadBackParticles();
        std::cout << std::format("Frame {}: FPS: {:.1f}, First particle: ({:.3f}, {:.3f}, {:.3f})\n",
            frameCount, frameCount / totalTime,
            particles[0].position.x, particles[0].position.y, particles[0].position.z);
    }
    catch (const std::exception& e) {
        std::cout << std::format("Frame {}: FPS: {:.1f} (Debug readback failed)\n",
            frameCount, frameCount / totalTime);
    }
}

void Application::Shutdown() {
    renderer.reset();
    particleSystem.reset();
    shaderManager.reset();
    device.reset();
    
    if (hWnd) {
        DestroyWindow(hWnd);
        hWnd = nullptr;
    }
}

LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CREATE) {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        Application* pApp = (Application*)pcs->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
        return 0;
    }

    Application* pApp = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pApp) {
        return pApp->HandleMessage(hWnd, message, wParam, lParam);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT Application::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            running = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            OnKeyDown(wParam);
            return 0;
            
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                OnResize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;
            
        case WM_MOUSEMOVE:
            OnMouseMove(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_LBUTTONDOWN:
            OnMouseDown(LOWORD(lParam), HIWORD(lParam), true, false);
            return 0;
            
        case WM_LBUTTONUP:
            OnMouseUp(LOWORD(lParam), HIWORD(lParam), true, false);
            return 0;
            
        case WM_RBUTTONDOWN:
            OnMouseDown(LOWORD(lParam), HIWORD(lParam), false, true);
            return 0;
            
        case WM_RBUTTONUP:
            OnMouseUp(LOWORD(lParam), HIWORD(lParam), false, true);
            return 0;
            
        case WM_MOUSEWHEEL:
            OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void Application::OnKeyDown(WPARAM key) {
    switch (key) {
        case VK_ESCAPE:
            running = false;
            break;
            
        case 'R':
            particleSystem->Reset();
            camera->ResetCamera();
            frameCount = 0;
            totalTime = 0.0f;
            LOG_INFO("Simulation and camera reset");
            break;
            
        case 'C':
            camera->ResetCamera();
            LOG_INFO("Camera reset to default position");
            break;
            
        case '1': case '2': case '3': case '4': case '5': 
        case '6': case '7': case '8': case '9':
            {
                int percentile = (key - '0') * 10;
                camera->SetZoomPercentile(percentile / 100.0f);
                LOG_INFO("Zoom set to {}%", percentile);
            }
            break;
            
        case 'P':
            // Toggle stats overlay display
            if (statsOverlay) {
                statsOverlay->Toggle();
                LOG_INFO("Stats overlay {}", statsOverlay->IsVisible() ? "enabled" : "disabled");
                if (statsOverlay->IsVisible()) {
                    statsOverlay->PrintStats();
                }
            }
            break;
            
        case 'L':
            // Toggle logging level
            {
                static int logLevel = 1;
                logLevel = (logLevel + 1) % 4;
                LogLevel levels[] = { LogLevel::Error, LogLevel::Warning, LogLevel::Info, LogLevel::Debug };
                Logger::GetInstance().SetLevel(levels[logLevel]);
                const char* names[] = { "Error", "Warning", "Info", "Debug" };
                LOG_INFO("Logging level set to {}", names[logLevel]);
            }
            break;
            
        case VK_SPACE:
            // Center camera on particle cloud
            camera->CenterOnCloud();
            LOG_INFO("Camera centered on particle cloud");
            break;
            
        case VK_RIGHT:
        case VK_ADD:
        case '=':
            // Next preset
            if (presetManager && presetManager->LoadNextPreset(simulationConfig, renderConfig)) {
                ApplyNewConfiguration();
            }
            break;
            
        case VK_LEFT:
        case VK_SUBTRACT:
        case '-':
            // Previous preset
            if (presetManager && presetManager->LoadPreviousPreset(simulationConfig, renderConfig)) {
                ApplyNewConfiguration();
            }
            break;
            
        case 'H':
            // Show help/presets
            if (presetManager) {
                presetManager->PrintAvailablePresets();
            }
            break;
    }
}

void Application::OnResize(int width, int height) {
    // In a more complete implementation, we would resize the swap chain
    std::cout << std::format("Window resized to {}x{}\n", width, height);
}

void Application::ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            running = false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Application::OnMouseMove(int x, int y) {
    if (camera) {
        camera->OnMouseMove(x, y);
    }
}

void Application::OnMouseDown(int x, int y, bool leftButton, bool rightButton) {
    if (camera) {
        camera->OnMouseDown(x, y, leftButton);
    }
    
    // Capture mouse for dragging
    if (leftButton) {
        SetCapture(hWnd);
    }
}

void Application::OnMouseUp(int x, int y, bool leftButton, bool rightButton) {
    if (camera) {
        camera->OnMouseUp(x, y, leftButton);
    }
    
    // Release mouse capture
    if (leftButton) {
        ReleaseCapture();
    }
}

void Application::OnMouseWheel(int delta) {
    if (camera) {
        camera->OnMouseWheel(delta);
    }
}

void Application::ApplyNewConfiguration() {
    LOG_INFO("Applying new configuration...");
    
    // Recreate particle system with new configuration
    particleSystem = std::make_shared<ParticleSystem>(device, simulationConfig);
    if (!particleSystem->Initialize()) {
        LOG_ERROR("Failed to reinitialize particle system with new configuration");
        return;
    }
    
    // Update renderer configuration
    renderer = std::make_shared<Renderer>(device, shaderManager, renderConfig);
    if (!renderer->Initialize()) {
        LOG_ERROR("Failed to reinitialize renderer with new configuration");
        return;
    }
    
    // Update camera configuration
    camera->SetConfig(renderConfig);
    camera->ResetCamera();
    
    // Reset timing and frame counters
    frameCount = 0;
    totalTime = 0.0f;
    lastFrameTime = std::chrono::high_resolution_clock::now();
    
    LOG_INFO("New configuration applied successfully");
}

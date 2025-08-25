/**
 * @file main.cpp
 * @brief Entry point for multi-type particle physics simulation
 * 
 * This is the main entry point for the DirectX 11 particle physics simulation
 * application. It initializes memory management, error handling, and creates
 * the main Application instance.
 * 
 * The application features:
 * - Multi-type particle system with force-based interactions
 * - GPU-accelerated physics simulation using compute shaders
 * - Real-time 3D rendering with icosphere geometry and Phong lighting
 * - Interactive camera controls with mouse and keyboard input
 * - Performance monitoring and statistics overlay
 * - Configurable simulation parameters via JSON files
 * 
 * Build requirements: DirectX 11, Windows 10+, Visual Studio 2019+
 */

#include "Application.h"
#include "Logger.h"
#include "MemoryManager.h"
#include "ErrorHandling.h"
#include <iostream>
#include <exception>

#ifdef _DEBUG
#include <crtdbg.h>
#endif

/**
 * Windows application entry point
 * 
 * Initializes memory management, error handling, and runs the main
 * particle simulation application with proper exception handling.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Suppress compiler warnings for unused parameters
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#ifdef _DEBUG
    // Enable run-time memory leak detection for debug builds
    // This will report memory leaks in the Visual Studio output window
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Initialize global memory manager for GPU buffer management and profiling
    MemoryManager::GetInstance().Initialize();
    
    // Initialize global error handling system with logging and crash reporting
    ErrorHandling::Initialize();
    
    try {
        // Create the main particle simulation application instance
        auto app = std::make_unique<Application>();
        
        // Initialize all subsystems (DirectX, shaders, physics, rendering)
        if (!app->Initialize()) {
            LOG_ERROR("Failed to initialize multi-type particle simulation");
            return -1;
        }
        
        // Enter the main application loop (Windows message processing and rendering)
        // This loop continues until the user exits the application
        app->Run();
        
        // Perform clean shutdown of all systems in reverse initialization order
        app->Shutdown();
        
        LOG_INFO("Multi-type particle simulation completed successfully");
        return 0;
    }
    catch (const std::exception& e) {
        // Handle any standard C++ exceptions with logging and console output
        LOG_ERROR("Unhandled exception in main: {}", e.what());
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        // Handle any non-standard exceptions (should not happen in well-written code)
        LOG_ERROR("Unknown unhandled exception in main");
        std::cerr << "Fatal error: Unknown exception" << std::endl;
        return -1;
    }
}

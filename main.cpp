#include "Application.h"
#include "Logger.h"
#include "MemoryManager.h"
#include "ErrorHandling.h"
#include <iostream>
#include <exception>

#ifdef _DEBUG
#include <crtdbg.h>
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#ifdef _DEBUG
    // Enable run-time memory check for debug builds
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Initialize memory manager
    MemoryManager::GetInstance().Initialize();
    
    // Initialize error handling
    ErrorHandling::Initialize();
    
    try {
        // Create and initialize clean multi-type particle application
        auto app = std::make_unique<Application>();
        
        if (!app->Initialize()) {
            LOG_ERROR("Failed to initialize multi-type particle simulation");
            return -1;
        }
        
        // Run main application loop
        app->Run();
        
        // Clean shutdown
        app->Shutdown();
        
        LOG_INFO("Multi-type particle simulation completed successfully");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Unhandled exception in main: {}", e.what());
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        LOG_ERROR("Unknown unhandled exception in main");
        std::cerr << "Fatal error: Unknown exception" << std::endl;
        return -1;
    }
}

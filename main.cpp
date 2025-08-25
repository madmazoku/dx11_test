#include "Application.h"
#include "Logger.h"
#include <iostream>
#include <filesystem>

int main() {
    // Initialize logger first
    Logger::GetInstance().Initialize("particle_simulation.log", LogLevel::Info, true);
    
    LOG_INFO("=== Particle Physics Simulation - Verlet Integration ===");
    LOG_INFO("Working directory: {}", std::filesystem::current_path().string());

    try {
        Application app;
        
        if (!app.Initialize()) {
            LOG_ERROR("Failed to initialize application");
            return 1;
        }
        
        app.Run();
        
        LOG_INFO("Application finished successfully");
        return 0;
    }
    catch (const std::exception& e) {
        LOG_CRITICAL("Unhandled exception: {}", e.what());
        return 1;
    }
    catch (...) {
        LOG_CRITICAL("Unknown exception occurred");
        return 1;
    }
}

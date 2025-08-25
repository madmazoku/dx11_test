#include "StatsOverlay.h"
#include "Logger.h"
#include <format>
#include <sstream>
#include <iomanip>

void StatsOverlay::Update(float deltaTime, float currentFPS, const ParticleSystem& particles, 
                         const InteractiveCamera& camera, float simulationTime) {
    timeSinceLastUpdate += deltaTime;
    
    if (timeSinceLastUpdate >= refreshInterval) {
        // Update cached statistics
        stats.fps = currentFPS;
        stats.frameTime = deltaTime * 1000.0f; // Convert to milliseconds
        stats.particleCount = particles.GetParticleCount();
        stats.simulationTime = simulationTime;
        stats.cameraPosition = camera.GetPosition();
        
        // Reset timer
        timeSinceLastUpdate = 0.0f;
    }
}

std::string StatsOverlay::GetStatsText() const {
    if (!visible) return "";
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    
    oss << "=== Particle Physics Simulation Stats ===\n";
    oss << "FPS: " << stats.fps << " (" << std::setprecision(2) << stats.frameTime << "ms)\n";
    oss << "Particles: " << stats.particleCount << "\n";
    oss << "Simulation Time: " << std::setprecision(1) << stats.simulationTime << "s\n";
    oss << "\nCamera:\n";
    oss << "  Position: (" << stats.cameraPosition.x << ", " << stats.cameraPosition.y << ", " << stats.cameraPosition.z << ")\n";
    oss << "  Distance: " << stats.cameraDistance << "\n";
    oss << "  Zoom: " << std::setprecision(0) << (stats.zoomPercentile * 100) << "%\n";
    oss << "\nControls:\n";
    oss << "  ESC - Exit\n";
    oss << "  R - Reset simulation\n";
    oss << "  C - Reset camera\n";
    oss << "  SPACE - Center camera\n";
    oss << "  Mouse wheel - Adjust zoom\n";
    oss << "  Left mouse + drag - Rotate camera\n";
    oss << "  1-9 keys - Set zoom to 10%-90%\n";
    oss << "  P - Toggle this display\n";
    
    return oss.str();
}

void StatsOverlay::PrintStats() const {
    if (!visible) return;
    
    std::string statsText = GetStatsText();
    
    // Print to console (Windows-specific)
#ifdef _WIN32
    system("cls"); // Clear console
#endif
    
    LOG_INFO("Performance Statistics:");
    LOG_INFO("FPS: {:.1f} ({:.2f}ms frame time)", stats.fps, stats.frameTime);
    LOG_INFO("Particles: {}", stats.particleCount);
    LOG_INFO("Simulation Time: {:.1f}s", stats.simulationTime);
    LOG_INFO("Camera Position: ({:.1f}, {:.1f}, {:.1f})", 
             stats.cameraPosition.x, stats.cameraPosition.y, stats.cameraPosition.z);
}

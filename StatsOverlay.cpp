#include "StatsOverlay.h"
#include "Renderer.h"
#include "Logger.h"
#include <format>
#include <sstream>
#include <iomanip>

void StatsOverlay::Update(float deltaTime, float currentFPS, const ParticleSystem& particles, 
                         const InteractiveCamera& camera, float simulationTime, const Renderer* renderer) {
    timeSinceLastUpdate += deltaTime;
    
    if (timeSinceLastUpdate >= refreshInterval) {
        // Update cached statistics
        stats.fps = currentFPS;
        stats.frameTime = deltaTime * 1000.0f; // Convert to milliseconds
        stats.particleCount = particles.GetParticleCount();
        stats.simulationTime = simulationTime;
        stats.cameraPosition = camera.GetPosition();
        
        // Update culling statistics if available
        if (renderer && renderer->GetFrustumCuller()) {
            stats.frustumCullingEnabled = renderer->IsFrustumCullingEnabled();
            auto cullingStats = renderer->GetFrustumCuller()->GetStats();
            stats.visibleParticles = cullingStats.visibleParticles;
            stats.culledByFrustum = cullingStats.culledByFrustum;
            stats.culledByDistance = cullingStats.culledByDistance;
            stats.lodLevel0Count = cullingStats.lodLevel0Count;
            stats.lodLevel1Count = cullingStats.lodLevel1Count;
            stats.lodLevel2Count = cullingStats.lodLevel2Count;
            stats.cullingTimeMs = cullingStats.cullingTimeMs;
        } else {
            stats.frustumCullingEnabled = false;
        }
        
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
    
    // Culling statistics
    if (stats.frustumCullingEnabled) {
        oss << "\nFrustum Culling:\n";
        oss << "  Status: Enabled\n";
        oss << "  Visible: " << stats.visibleParticles << "/" << stats.particleCount << "\n";
        oss << "  Culled by frustum: " << stats.culledByFrustum << "\n";
        oss << "  Culled by distance: " << stats.culledByDistance << "\n";
        oss << "  LOD High: " << stats.lodLevel0Count << "\n";
        oss << "  LOD Medium: " << stats.lodLevel1Count << "\n";
        oss << "  LOD Low: " << stats.lodLevel2Count << "\n";
        oss << "  Culling time: " << std::setprecision(2) << stats.cullingTimeMs << "ms\n";
    } else {
        oss << "\nFrustum Culling: Disabled\n";
    }
    
    oss << "\nControls:\n";
    oss << "  ESC - Exit\n";
    oss << "  R - Reset simulation\n";
    oss << "  C - Reset camera\n";
    oss << "  SPACE - Center camera\n";
    oss << "  Mouse wheel - Adjust zoom\n";
    oss << "  Left mouse + drag - Rotate camera\n";
    oss << "  1-9 keys - Set zoom to 10%-90%\n";
    oss << "  P - Toggle this display\n";
    oss << "  F - Toggle frustum culling\n";
    oss << "  T - Print culling statistics\n";
    
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

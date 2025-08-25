/**
 * @file StatsOverlay.cpp  
 * @brief Implementation of the StatsOverlay class for real-time performance monitoring
 * 
 * The StatsOverlay provides comprehensive real-time statistics display for the particle
 * simulation system. It tracks performance metrics, particle counts, camera state,
 * frustum culling efficiency, and provides an interactive overlay for runtime monitoring.
 * 
 * Statistics are updated at configurable intervals to balance accuracy with performance,
 * and can be displayed as on-screen overlays or logged to console for debugging.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#include "StatsOverlay.h"
#include "Renderer.h"
#include "Logger.h"
#include <format>
#include <sstream>
#include <iomanip>

/**
 * @brief Updates all statistics with current system state
 * 
 * Collects performance metrics, particle counts, camera state, and frustum culling
 * statistics from the simulation systems. Updates occur at the configured refresh
 * interval to balance accuracy with performance impact.
 * 
 * @param deltaTime Time elapsed since last frame in seconds
 * @param currentFPS Current frames per second measurement
 * @param particles Reference to particle system for count and state data
 * @param camera Reference to camera system for position and zoom data
 * @param simulationTime Total elapsed simulation time in seconds
 * @param renderer Pointer to renderer for culling statistics (optional)
 */
void StatsOverlay::Update(float deltaTime, float currentFPS, const ParticleSystem& particles, 
                         const InteractiveCamera& camera, float simulationTime, const Renderer* renderer) {
    timeSinceLastUpdate += deltaTime;
    
    // Update statistics only at the configured refresh interval for performance
    if (timeSinceLastUpdate >= refreshInterval) {
        // Update cached performance metrics
        stats.fps = currentFPS;
        stats.frameTime = deltaTime * 1000.0f; // Convert to milliseconds for readability
        stats.particleCount = particles.GetParticleCount();
        stats.simulationTime = simulationTime;
        stats.cameraPosition = camera.GetPosition();
        
        // Update culling statistics if renderer provides frustum culling data
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
        
        // Reset update timer for next interval
        timeSinceLastUpdate = 0.0f;
    }
}

/**
 * @brief Generates formatted statistics text for display overlay
 * 
 * Creates a comprehensive, human-readable statistics display including:
 * - Performance metrics (FPS, frame time)
 * - Particle system state (count, simulation time)
 * - Camera information (position, distance, zoom)
 * - Frustum culling efficiency (visible/culled counts, LOD distribution)
 * - Interactive control reference
 * 
 * @return Formatted string ready for overlay display, empty if overlay is hidden
 */
std::string StatsOverlay::GetStatsText() const {
    if (!visible) return "";
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    
    // Header and basic performance metrics
    oss << "=== Particle Physics Simulation Stats ===\n";
    oss << "FPS: " << stats.fps << " (" << std::setprecision(2) << stats.frameTime << "ms)\n";
    oss << "Particles: " << stats.particleCount << "\n";
    oss << "Simulation Time: " << std::setprecision(1) << stats.simulationTime << "s\n";
    
    // Camera state information
    oss << "\nCamera:\n";
    oss << "  Position: (" << stats.cameraPosition.x << ", " << stats.cameraPosition.y << ", " << stats.cameraPosition.z << ")\n";
    oss << "  Distance: " << stats.cameraDistance << "\n";
    oss << "  Zoom: " << std::setprecision(0) << (stats.zoomPercentile * 100) << "%\n";
    
    // Detailed frustum culling performance statistics
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
    
    // Interactive control reference for user convenience  
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

/**
 * @brief Prints key statistics to console/log for debugging
 * 
 * Outputs essential performance metrics to the logging system for debugging
 * and profiling purposes. Clears console on Windows platforms for cleaner output.
 * Only prints if the overlay is currently visible.
 */
void StatsOverlay::PrintStats() const {
    if (!visible) return;
    
    std::string statsText = GetStatsText();
    
    // Clear console for cleaner output (Windows-specific optimization)
#ifdef _WIN32
    system("cls"); // Clear console
#endif
    
    // Log essential performance metrics for debugging
    LOG_INFO("Performance Statistics:");
    LOG_INFO("FPS: {:.1f} ({:.2f}ms frame time)", stats.fps, stats.frameTime);
    LOG_INFO("Particles: {}", stats.particleCount);
    LOG_INFO("Simulation Time: {:.1f}s", stats.simulationTime);
    LOG_INFO("Camera Position: ({:.1f}, {:.1f}, {:.1f})", 
             stats.cameraPosition.x, stats.cameraPosition.y, stats.cameraPosition.z);
}

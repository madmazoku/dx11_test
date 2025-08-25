/**
 * @file StatsOverlay.h
 * @brief Header for the StatsOverlay class - real-time performance monitoring and statistics display
 * 
 * The StatsOverlay provides comprehensive real-time statistics for the particle simulation
 * system, including performance metrics, particle counts, camera state, and frustum culling
 * efficiency. It supports both on-screen overlay display and console logging for debugging.
 * 
 * Statistics are cached and updated at configurable intervals to balance monitoring
 * accuracy with performance impact. The overlay can be toggled on/off during runtime
 * and provides detailed information useful for performance optimization and debugging.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#pragma once

#include "Structures.h"
#include "ParticleSystem.h"
#include "InteractiveCamera.h"
#include <string>
#include <memory>

/**
 * @class StatsOverlay
 * @brief Real-time statistics display and monitoring system
 * 
 * Provides comprehensive runtime statistics for the particle simulation including:
 * - Performance metrics (FPS, frame time, simulation time)
 * - Particle system state (count, distribution, physics parameters)
 * - Camera information (position, distance, zoom level)
 * - Frustum culling efficiency (visible/culled counts, LOD distribution)
 * - GPU performance metrics (culling time, render statistics)
 * 
 * The overlay uses cached statistics updated at configurable intervals to minimize
 * performance impact while providing accurate monitoring data. It supports both
 * graphical overlay display and console logging for different use cases.
 */
class StatsOverlay {
private:
    bool visible = false;                    ///< Whether the overlay is currently visible
    float refreshInterval = 0.5f;           ///< Statistics refresh interval in seconds (500ms default)
    float timeSinceLastUpdate = 0.0f;       ///< Time accumulator for refresh timing
    
    /**
     * @struct Stats
     * @brief Cached statistics data structure
     * 
     * Contains all monitored statistics that are updated at the refresh interval
     * to provide consistent, low-overhead access to performance data.
     */
    struct Stats {
        // Core performance metrics
        float fps = 0.0f;                    ///< Current frames per second
        float frameTime = 0.0f;              ///< Frame time in milliseconds
        int particleCount = 0;               ///< Total number of active particles
        float simulationTime = 0.0f;         ///< Total elapsed simulation time in seconds
        
        // Camera state information
        XMFLOAT3 cameraPosition = {0.0f, 0.0f, 0.0f}; ///< Current camera world position
        XMFLOAT3 cloudCenter = {0.0f, 0.0f, 0.0f};    ///< Center of particle cloud (for reference)
        float zoomPercentile = 1.0f;         ///< Current zoom level as percentile (0.0-1.0)
        float cameraDistance = 0.0f;         ///< Distance from camera to target/center
        
        // Frustum culling performance statistics
        bool frustumCullingEnabled = false;  ///< Whether frustum culling is currently active
        int visibleParticles = 0;            ///< Number of particles visible to camera
        int culledByFrustum = 0;             ///< Particles culled by frustum bounds
        int culledByDistance = 0;            ///< Particles culled by distance limits
        int lodLevel0Count = 0;              ///< High-detail LOD particle count  
        int lodLevel1Count = 0;              ///< Medium-detail LOD particle count
        int lodLevel2Count = 0;              ///< Low-detail LOD particle count
        float cullingTimeMs = 0.0f;          ///< GPU time spent on culling operations
    } stats;

public:
    /**
     * @brief Constructs a new StatsOverlay with default settings
     */
    StatsOverlay() = default;
    ~StatsOverlay() = default;
    
    /**
     * @brief Updates all statistics with current system state
     * 
     * Collects performance metrics, particle counts, camera state, and frustum culling
     * statistics. Updates occur only at the configured refresh interval for performance.
     * 
     * @param deltaTime Time elapsed since last frame in seconds
     * @param currentFPS Current frames per second measurement
     * @param particles Reference to particle system for count and state data
     * @param camera Reference to camera system for position and zoom data  
     * @param simulationTime Total elapsed simulation time in seconds
     * @param renderer Optional pointer to renderer for culling statistics
     */
    void Update(float deltaTime, float currentFPS, const ParticleSystem& particles, 
                const InteractiveCamera& camera, float simulationTime, const class Renderer* renderer = nullptr);
    
    /**
     * @brief Toggles overlay visibility on/off
     */
    void Toggle() { visible = !visible; }
    
    /**
     * @brief Sets overlay visibility state
     * @param vis Whether overlay should be visible
     */
    void SetVisible(bool vis) { visible = vis; }
    
    /**
     * @brief Checks if overlay is currently visible
     * @return true if overlay is visible, false otherwise
     */
    bool IsVisible() const { return visible; }
    
    /**
     * @brief Generates formatted statistics text for display
     * @return Formatted string ready for overlay rendering, empty if hidden
     */
    std::string GetStatsText() const;
    
    /**
     * @brief Prints key statistics to console/log for debugging
     */
    void PrintStats() const;
};

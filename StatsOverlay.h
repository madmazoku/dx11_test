#pragma once

#include "Structures.h"
#include "ParticleSystem.h"
#include "InteractiveCamera.h"
#include <string>
#include <memory>

class StatsOverlay {
private:
    bool visible = false;
    float refreshInterval = 0.5f; // Refresh every 500ms
    float timeSinceLastUpdate = 0.0f;
    
    // Cached statistics
    struct Stats {
        float fps = 0.0f;
        float frameTime = 0.0f;
        int particleCount = 0;
        float simulationTime = 0.0f;
        XMFLOAT3 cameraPosition = {0.0f, 0.0f, 0.0f};
        XMFLOAT3 cloudCenter = {0.0f, 0.0f, 0.0f};
        float zoomPercentile = 1.0f;
        float cameraDistance = 0.0f;
        
        // Culling stats
        bool frustumCullingEnabled = false;
        int visibleParticles = 0;
        int culledByFrustum = 0;
        int culledByDistance = 0;
        int lodLevel0Count = 0;
        int lodLevel1Count = 0;
        int lodLevel2Count = 0;
        float cullingTimeMs = 0.0f;
    } stats;

public:
    StatsOverlay() = default;
    ~StatsOverlay() = default;
    
    void Update(float deltaTime, float currentFPS, const ParticleSystem& particles, 
                const InteractiveCamera& camera, float simulationTime, const class Renderer* renderer = nullptr);
    
    void Toggle() { visible = !visible; }
    void SetVisible(bool vis) { visible = vis; }
    bool IsVisible() const { return visible; }
    
    std::string GetStatsText() const;
    void PrintStats() const;
};

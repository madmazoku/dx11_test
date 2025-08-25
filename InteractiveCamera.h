#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <vector>
#include "Structures.h"

using namespace DirectX;

struct CameraState {
    XMFLOAT3 position;
    XMFLOAT3 target;
    XMFLOAT3 up;
    float distance;
    float theta;  // Horizontal rotation angle
    float phi;    // Vertical rotation angle
    float zoomPercentile; // 1% to 100%
};

class InteractiveCamera {
private:
    CameraState state;
    RenderConfig config;
    
    // Mouse interaction state
    bool isDragging;
    POINT lastMousePos;
    POINT currentMousePos;
    
    // Particle cloud bounds
    XMFLOAT3 cloudCenter;
    float cloudRadius;
    std::vector<float> particleDistances; // For percentile calculation

public:
    InteractiveCamera(const RenderConfig& config);
    ~InteractiveCamera() = default;
    
    // Update camera based on particle positions
    void UpdateCloudBounds(const std::vector<Particle>& particles);
    void UpdateCloudBounds(const XMFLOAT3* positions, size_t count);
    
    // Mouse interaction
    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y, bool leftButton);
    void OnMouseUp(int x, int y, bool leftButton);
    void OnMouseWheel(int delta);
    
    // Camera transformation
    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix(float aspectRatio) const;
    XMFLOAT3 GetPosition() const { return state.position; }
    XMFLOAT3 GetTarget() const { return state.target; }
    
    // Configuration
    void SetConfig(const RenderConfig& newConfig) { config = newConfig; }
    void SetZoomPercentile(float percentile); // 0.01 to 1.0
    void SetRotation(float theta, float phi);
    void ResetCamera();
    
    // Auto-centering
    void CenterOnCloud();
    float CalculateOptimalDistance() const;
    
private:
    void UpdateCameraPosition();
    void ClampAngles();
    float CalculatePercentileDistance(float percentile) const;
};

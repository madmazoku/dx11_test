#include "InteractiveCamera.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

InteractiveCamera::InteractiveCamera(const RenderConfig& config) : config(config) {
    // Initialize camera state
    state.target = { 0.0f, 0.0f, 0.0f };
    state.up = { 0.0f, 1.0f, 0.0f };
    state.distance = config.cameraRadius;
    state.theta = 0.0f;      // Horizontal angle
    state.phi = XM_PI * 0.25f; // Vertical angle (45 degrees)
    state.zoomPercentile = 0.95f; // Show 95% of particles by default
    
    isDragging = false;
    cloudCenter = { 0.0f, 0.0f, 0.0f };
    cloudRadius = 10.0f;
    
    UpdateCameraPosition();
    
    LOG_INFO("Interactive camera initialized");
}

void InteractiveCamera::UpdateCloudBounds(const std::vector<Particle>& particles) {
    if (particles.empty()) return;
    
    // Calculate center of mass
    XMFLOAT3 centerSum = { 0.0f, 0.0f, 0.0f };
    for (const auto& particle : particles) {
        centerSum.x += particle.position.x;
        centerSum.y += particle.position.y;
        centerSum.z += particle.position.z;
    }
    
    cloudCenter.x = centerSum.x / particles.size();
    cloudCenter.y = centerSum.y / particles.size();
    cloudCenter.z = centerSum.z / particles.size();
    
    // Calculate distances from center for percentile calculation
    particleDistances.clear();
    particleDistances.reserve(particles.size());
    
    for (const auto& particle : particles) {
        float dx = particle.position.x - cloudCenter.x;
        float dy = particle.position.y - cloudCenter.y;
        float dz = particle.position.z - cloudCenter.z;
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        particleDistances.push_back(distance);
    }
    
    // Sort for percentile calculation
    std::sort(particleDistances.begin(), particleDistances.end());
    
    // Calculate max radius (for fallback)
    cloudRadius = particleDistances.empty() ? 10.0f : particleDistances.back();
    
    // Update camera target to cloud center if auto-centering is enabled
    if (config.enableAutoCentering) {
        CenterOnCloud();
    }
}

void InteractiveCamera::UpdateCloudBounds(const XMFLOAT3* positions, size_t count) {
    if (count == 0) return;
    
    // Calculate center of mass
    XMFLOAT3 centerSum = { 0.0f, 0.0f, 0.0f };
    for (size_t i = 0; i < count; ++i) {
        centerSum.x += positions[i].x;
        centerSum.y += positions[i].y;
        centerSum.z += positions[i].z;
    }
    
    cloudCenter.x = centerSum.x / count;
    cloudCenter.y = centerSum.y / count;
    cloudCenter.z = centerSum.z / count;
    
    // Calculate distances from center
    particleDistances.clear();
    particleDistances.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        float dx = positions[i].x - cloudCenter.x;
        float dy = positions[i].y - cloudCenter.y;
        float dz = positions[i].z - cloudCenter.z;
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        particleDistances.push_back(distance);
    }
    
    std::sort(particleDistances.begin(), particleDistances.end());
    cloudRadius = particleDistances.empty() ? 10.0f : particleDistances.back();
    
    if (config.enableAutoCentering) {
        CenterOnCloud();
    }
}

void InteractiveCamera::OnMouseMove(int x, int y) {
    currentMousePos.x = x;
    currentMousePos.y = y;
    
    if (isDragging) {
        int deltaX = x - lastMousePos.x;
        int deltaY = y - lastMousePos.y;
        
        // Update rotation angles
        state.theta += deltaX * config.cameraRotationSensitivity;
        state.phi -= deltaY * config.cameraRotationSensitivity;
        
        ClampAngles();
        UpdateCameraPosition();
        
        lastMousePos = currentMousePos;
    }
}

void InteractiveCamera::OnMouseDown(int x, int y, bool leftButton) {
    if (leftButton) {
        isDragging = true;
        lastMousePos.x = x;
        lastMousePos.y = y;
        currentMousePos = lastMousePos;
        
        LOG_DEBUG("Camera rotation started at ({}, {})", x, y);
    }
}

void InteractiveCamera::OnMouseUp(int x, int y, bool leftButton) {
    if (leftButton) {
        isDragging = false;
        LOG_DEBUG("Camera rotation stopped");
    }
}

void InteractiveCamera::OnMouseWheel(int delta) {
    // Convert wheel delta to zoom change
    float zoomChange = (delta > 0 ? -config.cameraZoomStep : config.cameraZoomStep);
    
    // Update zoom percentile
    state.zoomPercentile = std::clamp(
        state.zoomPercentile + zoomChange,
        config.cameraZoomMin,
        config.cameraZoomMax
    );
    
    // Recalculate optimal distance
    state.distance = CalculateOptimalDistance();
    UpdateCameraPosition();
    
    LOG_DEBUG("Camera zoom: {:.1f}% (distance: {:.2f})", 
        state.zoomPercentile * 100.0f, state.distance);
}

XMMATRIX InteractiveCamera::GetViewMatrix() const {
    XMVECTOR eye = XMLoadFloat3(&state.position);
    XMVECTOR target = XMLoadFloat3(&state.target);
    XMVECTOR up = XMLoadFloat3(&state.up);
    return XMMatrixLookAtLH(eye, target, up);
}

XMMATRIX InteractiveCamera::GetProjectionMatrix(float aspectRatio) const {
    float fovY = XMConvertToRadians(config.cameraFov);
    return XMMatrixPerspectiveFovLH(fovY, aspectRatio, config.cameraNearPlane, config.cameraFarPlane);
}

void InteractiveCamera::SetZoomPercentile(float percentile) {
    state.zoomPercentile = std::clamp(percentile, config.cameraZoomMin, config.cameraZoomMax);
    state.distance = CalculateOptimalDistance();
    UpdateCameraPosition();
}

void InteractiveCamera::SetRotation(float theta, float phi) {
    state.theta = theta;
    state.phi = phi;
    ClampAngles();
    UpdateCameraPosition();
}

void InteractiveCamera::ResetCamera() {
    state.theta = 0.0f;
    state.phi = XM_PI * 0.25f;
    state.zoomPercentile = 0.95f;
    state.distance = CalculateOptimalDistance();
    CenterOnCloud();
    UpdateCameraPosition();
    
    LOG_INFO("Camera reset to default position");
}

void InteractiveCamera::CenterOnCloud() {
    state.target = cloudCenter;
    UpdateCameraPosition();
}

float InteractiveCamera::CalculateOptimalDistance() const {
    float percentileRadius = CalculatePercentileDistance(state.zoomPercentile);
    
    // Add some padding and account for FOV
    float fovRadians = XMConvertToRadians(config.cameraFov);
    float padding = 1.5f; // Extra space around particles
    
    // Calculate distance needed to fit the percentile radius in view
    float distance = (percentileRadius * padding) / std::tan(fovRadians * 0.5f);
    
    // Ensure minimum distance
    return std::max(distance, 1.0f);
}

void InteractiveCamera::UpdateCameraPosition() {
    // Convert spherical coordinates to Cartesian
    float x = state.distance * std::sin(state.phi) * std::cos(state.theta);
    float y = state.distance * std::cos(state.phi);
    float z = state.distance * std::sin(state.phi) * std::sin(state.theta);
    
    state.position.x = state.target.x + x;
    state.position.y = state.target.y + y;
    state.position.z = state.target.z + z;
}

void InteractiveCamera::ClampAngles() {
    // Keep theta in [0, 2π]
    while (state.theta < 0.0f) state.theta += XM_2PI;
    while (state.theta >= XM_2PI) state.theta -= XM_2PI;
    
    // Keep phi in [0.1, π-0.1] to avoid gimbal lock
    state.phi = std::clamp(state.phi, 0.1f, XM_PI - 0.1f);
}

float InteractiveCamera::CalculatePercentileDistance(float percentile) const {
    if (particleDistances.empty()) return cloudRadius;
    
    // Calculate index for percentile
    size_t index = static_cast<size_t>((particleDistances.size() - 1) * percentile);
    index = std::min(index, particleDistances.size() - 1);
    
    return particleDistances[index];
}

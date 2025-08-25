#pragma once

#include <DirectXMath.h>
#include <array>

using namespace DirectX;

struct SimulationConfig {
    size_t particleCount = 128;
    float springConstant = 50.0f;
    float restLength = 0.5f;
    float timeStep = 0.016f;  // 60fps
    float damping = 0.995f;
    float gravity[3] = { 0.0f, -9.81f, 0.0f };
    float boundaryMin[3] = { -5.0f, -5.0f, -5.0f };
    float boundaryMax[3] = { 5.0f, 5.0f, 5.0f };
    int maxFrames = 7200;
    bool enableDebugOutput = true;
    int debugOutputInterval = 60;
};

struct RenderConfig {
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    bool enableVSync = true;
    float sphereRadius = 0.1f;
    float clearColor[4] = { 0.1f, 0.1f, 0.2f, 1.0f };
    
    // Camera settings
    float cameraRadius = 12.0f;
    float cameraHeight = 5.0f;
    float cameraRotationSpeed = 0.5f;
    float cameraFov = 45.0f;
    float cameraNearPlane = 0.1f;
    float cameraFarPlane = 100.0f;
    
    // Interactive camera settings
    bool enableInteractiveCamera = true;
    float cameraZoomMin = 0.01f;    // 1% percentile zoom
    float cameraZoomMax = 1.0f;     // 100% percentile zoom  
    float cameraZoomStep = 0.1f;    // Mouse wheel sensitivity
    float cameraRotationSensitivity = 0.005f;  // Mouse rotation sensitivity
    bool enableAutoCentering = true; // Center camera on particle cloud
    
    // Lighting settings
    float lightDirection[3] = { -1.0f, -1.0f, -1.0f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float lightIntensity = 1.0f;
    float ambientIntensity = 0.2f;
    float specularPower = 32.0f;
};

// Particle structure for compute shader
struct Particle {
    XMFLOAT3 position;
    XMFLOAT3 oldPosition;
    XMFLOAT3 acceleration;
    float padding; // Align to 16 bytes
};

// Compute shader constant buffer
struct alignas(16) SimulationConstants {
    float springConstant;
    float restLength;
    float timeStep;
    float damping;
    XMFLOAT3 gravity;
    float padding1;
    XMFLOAT3 boundaryMin;
    float padding2;
    XMFLOAT3 boundaryMax;
    float padding3;
};

// Constant buffer structures
struct alignas(16) TransformBuffer {
    XMMATRIX viewProjectionMatrix;
    XMMATRIX worldMatrix;
    XMFLOAT3 cameraPos;
    float sphereRadius;
};

struct alignas(16) LightBuffer {
    XMFLOAT3 lightDirection;
    float lightIntensity;
    XMFLOAT3 lightColor;
    float ambientIntensity;
    XMFLOAT3 cameraPos;
    float specularPower;
};

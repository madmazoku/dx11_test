#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "ParticleSystem.h"
#include "InteractiveCamera.h"
#include "Structures.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>

using namespace DirectX;

class Renderer {
private:
    std::shared_ptr<D3DDevice> device;
    std::shared_ptr<ShaderManager> shaderManager;
    RenderConfig config;
    
    // Constant buffers
    ComPtr<ID3D11Buffer> transformBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;
    ComPtr<ID3D11Buffer> simulationConstantsBuffer;
    
    // Render states
    ComPtr<ID3D11RasterizerState> rasterizerState;
    ComPtr<ID3D11DepthStencilState> depthStencilState;
    ComPtr<ID3D11BlendState> blendState;
    
    // Camera parameters
    XMFLOAT3 cameraPosition;
    float cameraDistance = 10.0f;
    float cameraAngle = 0.0f;

public:
    Renderer(std::shared_ptr<D3DDevice> device, 
             std::shared_ptr<ShaderManager> shaderManager,
             const RenderConfig& config);
    ~Renderer() = default;

    bool Initialize();
    void RenderFrame(const ParticleSystem& particleSystem, float deltaTime);
    void RenderFrameWithCamera(const ParticleSystem& particleSystem, 
                              const InteractiveCamera& camera, float deltaTime);
    
    // Camera control (legacy)
    void SetCameraDistance(float distance) { cameraDistance = distance; }
    void SetCameraAngle(float angle) { cameraAngle = angle; }

private:
    bool CreateConstantBuffers();
    bool CreateRenderStates();
    void UpdateTransformBuffer(float time);
    void UpdateTransformBufferWithCamera(const InteractiveCamera& camera, float time);
    void UpdateLightBuffer();
    void UpdateLightBufferWithCamera(const InteractiveCamera& camera);
    void UpdateSimulationConstantsBuffer(const ParticleSystem& particleSystem);
    void UpdateCameraPosition(float time);
    void RunComputeShader(const ParticleSystem& particleSystem);
    
    XMMATRIX CreateViewMatrix() const;
    XMMATRIX CreateProjectionMatrix() const;
};

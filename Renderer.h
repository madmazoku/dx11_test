#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "ConfigManager.h"
#include "FrustumCuller.h"
#include "Structures.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

using namespace DirectX;

class Renderer {
private:
    std::shared_ptr<D3DDevice> device;
    std::shared_ptr<ShaderManager> shaderManager;
    std::shared_ptr<ConfigManager> configManager;
    std::unique_ptr<FrustumCuller> frustumCuller;
    
    // Material data for all particle types
    std::vector<MaterialProperties> materials;
    
    // Constant buffers
    ComPtr<ID3D11Buffer> transformBuffer;
    ComPtr<ID3D11Buffer> lightBuffer;
    ComPtr<ID3D11Buffer> simulationConstantsBuffer;
    ComPtr<ID3D11Buffer> materialBuffer;
    ComPtr<ID3D11Buffer> cullingBuffer;
    
    // Shader resource views
    ComPtr<ID3D11ShaderResourceView> materialBufferSRV;
    
    // Render states
    ComPtr<ID3D11RasterizerState> rasterizerState;
    ComPtr<ID3D11DepthStencilState> depthStencilState;
    ComPtr<ID3D11BlendState> blendState;
    
    // Camera parameters
    XMFLOAT3 cameraPosition;
    float cameraDistance = 10.0f;
    float cameraAngle = 0.0f;
    
    // Culling settings
    bool enableFrustumCulling = true;

    // Helper methods
    bool CreateConstantBuffers();
    bool CreateMaterialBuffer();
    bool CreateRenderStates();
    bool CreateCullingBuffer();
    
    void UpdateTransformBuffer(const XMMATRIX& viewMatrix, 
                              const XMMATRIX& projectionMatrix, 
                              const XMFLOAT3& cameraPos);
    void UpdateLightBuffer(const XMFLOAT3& cameraPos);
    
    ID3D11Buffer** GetCullingBuffer();

public:
    Renderer(std::shared_ptr<D3DDevice> device, 
             std::shared_ptr<ShaderManager> shaderManager,
             std::shared_ptr<ConfigManager> configManager);
    ~Renderer() = default;

    bool Initialize();
    bool InitializeWithMaxParticles(size_t maxParticles);
    
    // Multi-type particle rendering with icosphere generation and Phong shading
    void RenderFrameWithCamera(const std::vector<Particle>& particles, 
                              const XMMATRIX& viewMatrix, 
                              const XMMATRIX& projectionMatrix,
                              const XMFLOAT3& cameraPos);
    
    // Simulation update
    void UpdateSimulationConstants(const std::vector<Particle>& particles);
    
    // Camera control
    void SetCameraPosition(const XMFLOAT3& position);
    XMFLOAT3 GetCameraPosition() const;
    
    // Culling control
    void EnableFrustumCulling(bool enable);
    bool IsFrustumCullingEnabled() const { return enableFrustumCulling; }
    
    // Material system
    const std::vector<MaterialProperties>& GetMaterials() const { return materials; }
    void UpdateMaterials(const std::vector<MaterialProperties>& newMaterials);
};

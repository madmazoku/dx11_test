#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "InteractiveCamera.h"
#include "Structures.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

// Frustum culling data structure (matches HLSL)
struct CullingData {
    UINT isVisible;
    float distanceToCamera;
    UINT lodLevel;
    UINT padding;
};

// Frustum constant buffer (matches HLSL)
struct alignas(16) FrustumConstants {
    XMFLOAT4 frustumPlanes[6];  // Left, Right, Top, Bottom, Near, Far
    XMFLOAT3 cameraPosition;
    float maxRenderDistance;
    XMFLOAT3 cameraForward;
    float nearPlane;
    XMFLOAT3 cameraUp;
    float farPlane;
    XMFLOAT3 cameraRight;
    float fovY;
    float lodDistance1;    // High detail distance
    float lodDistance2;    // Medium detail distance  
    float lodDistance3;    // Low detail distance
    float aspectRatio;
};

class FrustumCuller {
private:
    std::shared_ptr<D3DDevice> device;
    std::shared_ptr<ShaderManager> shaderManager;
    
    // GPU resources
    ComPtr<ID3D11Buffer> frustumConstantsBuffer;
    ComPtr<ID3D11Buffer> cullingDataBuffer;
    ComPtr<ID3D11Buffer> cullingDataStagingBuffer;
    ComPtr<ID3D11ShaderResourceView> cullingDataSRV;
    ComPtr<ID3D11UnorderedAccessView> cullingDataUAV;
    
    // Culling settings
    struct CullingSettings {
        float maxRenderDistance = 100.0f;
        float lodDistance1 = 10.0f;   // High detail
        float lodDistance2 = 25.0f;   // Medium detail
        float lodDistance3 = 50.0f;   // Low detail
        bool enableLOD = true;
        bool enableBackfaceCulling = true;
        bool enableDistanceCulling = true;
    } settings;
    
    size_t particleCount = 0;
    std::vector<CullingData> cullingResults;
    
    // Statistics
    struct CullingStats {
        UINT totalParticles = 0;
        UINT visibleParticles = 0;
        UINT culledByFrustum = 0;
        UINT culledByDistance = 0;
        UINT culledByBackface = 0;
        UINT lodLevel0Count = 0; // High detail
        UINT lodLevel1Count = 0; // Medium detail
        UINT lodLevel2Count = 0; // Low detail
        float cullingTimeMs = 0.0f;
    } stats;

public:
    FrustumCuller(std::shared_ptr<D3DDevice> device, 
                  std::shared_ptr<ShaderManager> shaderManager);
    ~FrustumCuller() = default;
    
    bool Initialize(size_t maxParticles);
    
    // Main culling operation
    void PerformCulling(const InteractiveCamera& camera, 
                       ID3D11ShaderResourceView* particlesSRV,
                       float aspectRatio);
    
    // Results access
    ID3D11ShaderResourceView* GetCullingDataSRV() const { return cullingDataSRV.get(); }
    const CullingStats& GetStats() const { return stats; }
    
    // Settings
    void SetMaxRenderDistance(float distance) { settings.maxRenderDistance = distance; }
    void SetLODDistances(float lod1, float lod2, float lod3) {
        settings.lodDistance1 = lod1;
        settings.lodDistance2 = lod2;
        settings.lodDistance3 = lod3;
    }
    void EnableLOD(bool enable) { settings.enableLOD = enable; }
    void EnableBackfaceCulling(bool enable) { settings.enableBackfaceCulling = enable; }
    void EnableDistanceCulling(bool enable) { settings.enableDistanceCulling = enable; }
    
    // Debug
    std::vector<CullingData> ReadBackCullingResults();
    void PrintCullingStats() const;
    
private:
    bool CreateBuffers(size_t maxParticles);
    void UpdateFrustumConstants(const InteractiveCamera& camera, float aspectRatio);
    void ExtractFrustumPlanes(const XMMATRIX& viewProjectionMatrix, XMFLOAT4 planes[6]);
    void UpdateStatistics();
    
    // Frustum plane extraction helper
    XMFLOAT4 NormalizePlane(const XMFLOAT4& plane);
};

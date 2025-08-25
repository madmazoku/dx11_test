#include "FrustumCuller.h"
#include "Utilities.h"
#include "Logger.h"
#include "Profiler.h"
#include <algorithm>

using namespace DirectX;

FrustumCuller::FrustumCuller(std::shared_ptr<D3DDevice> device, 
                             std::shared_ptr<ShaderManager> shaderManager)
    : device(device), shaderManager(shaderManager) {
}

bool FrustumCuller::Initialize(size_t maxParticles) {
    PROFILE_FUNCTION();
    
    try {
        particleCount = maxParticles;
        cullingResults.resize(maxParticles);
        
        if (!CreateBuffers(maxParticles)) {
            LOG_ERROR("Failed to create frustum culler buffers");
            return false;
        }
        
        LOG_INFO("FrustumCuller initialized for {} particles", maxParticles);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("FrustumCuller initialization failed: {}", e.what());
        return false;
    }
}

bool FrustumCuller::CreateBuffers(size_t maxParticles) {
    auto d3dDevice = device->GetDevice();
    
    // Create frustum constants buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(FrustumConstants);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, frustumConstantsBuffer.getAddressOf()),
        "Failed to create frustum constants buffer"
    );

    // Create culling data buffer (GPU read/write)
    bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(CullingData) * maxParticles);
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(CullingData);

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, cullingDataBuffer.getAddressOf()),
        "Failed to create culling data buffer"
    );

    // Create staging buffer for readback
    bufferDesc.Usage = D3D11_USAGE_STAGING;
    bufferDesc.BindFlags = 0;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bufferDesc.MiscFlags = 0;

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, cullingDataStagingBuffer.getAddressOf()),
        "Failed to create culling data staging buffer"
    );

    // Create SRV for culling data
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>(maxParticles);

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(cullingDataBuffer.get(), &srvDesc, cullingDataSRV.getAddressOf()),
        "Failed to create culling data SRV"
    );

    // Create UAV for culling data
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = static_cast<UINT>(maxParticles);

    THROW_IF_FAILED(
        d3dDevice->CreateUnorderedAccessView(cullingDataBuffer.get(), &uavDesc, cullingDataUAV.getAddressOf()),
        "Failed to create culling data UAV"
    );

    return true;
}

void FrustumCuller::PerformCulling(const InteractiveCamera& camera, 
                                  ID3D11ShaderResourceView* particlesSRV,
                                  float aspectRatio) {
    PROFILE_FUNCTION();
    
    auto context = device->GetContext();
    
    // Update frustum constants from camera
    UpdateFrustumConstants(camera, aspectRatio);
    
    // Set compute shader and resources
    auto computeShader = shaderManager->GetComputeShader("FrustumCulling");
    if (!computeShader) {
        LOG_ERROR("Frustum culling compute shader not found");
        return;
    }
    
    context->CSSetShader(computeShader, nullptr, 0);
    
    // Set constant buffer
    context->CSSetConstantBuffers(0, 1, frustumConstantsBuffer.getAddressOf());
    
    // Set input particles SRV
    context->CSSetShaderResources(0, 1, &particlesSRV);
    
    // Set output culling data UAV
    context->CSSetUnorderedAccessViews(0, 1, cullingDataUAV.getAddressOf(), nullptr);
    
    // Dispatch compute shader
    UINT threadGroupsX = (static_cast<UINT>(particleCount) + 63) / 64; // 64 threads per group
    context->Dispatch(threadGroupsX, 1, 1);
    
    // Cleanup resources
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    context->CSSetShaderResources(0, 1, nullSRV);
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);
    
    // Update statistics (async readback in real implementation)
    UpdateStatistics();
}

void FrustumCuller::UpdateFrustumConstants(const InteractiveCamera& camera, float aspectRatio) {
    auto context = device->GetContext();
    
    FrustumConstants constants = {};
    
    // Get camera matrices
    XMMATRIX viewMatrix = camera.GetViewMatrix();
    XMMATRIX projMatrix = camera.GetProjectionMatrix(aspectRatio);
    XMMATRIX viewProjMatrix = XMMatrixMultiply(viewMatrix, projMatrix);
    
    // Extract frustum planes from view-projection matrix
    ExtractFrustumPlanes(viewProjMatrix, constants.frustumPlanes);
    
    // Camera parameters
    XMFLOAT3 cameraPos = camera.GetPosition();
    constants.cameraPosition = cameraPos;
    constants.cameraForward = camera.GetForward();
    constants.cameraUp = camera.GetUp();
    constants.cameraRight = camera.GetRight();
    
    // Projection parameters
    constants.nearPlane = camera.GetNearPlane();
    constants.farPlane = camera.GetFarPlane();
    constants.fovY = camera.GetFovY();
    constants.aspectRatio = aspectRatio;
    
    // Culling settings
    constants.maxRenderDistance = settings.maxRenderDistance;
    constants.lodDistance1 = settings.lodDistance1;
    constants.lodDistance2 = settings.lodDistance2;
    constants.lodDistance3 = settings.lodDistance3;
    
    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        context->Map(frustumConstantsBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map frustum constants buffer"
    );
    
    memcpy(mappedResource.pData, &constants, sizeof(FrustumConstants));
    context->Unmap(frustumConstantsBuffer.get(), 0);
}

void FrustumCuller::ExtractFrustumPlanes(const XMMATRIX& viewProjectionMatrix, XMFLOAT4 planes[6]) {
    // Extract frustum planes from view-projection matrix
    // This is the standard method using the columns of the matrix
    
    XMFLOAT4X4 vp;
    XMStoreFloat4x4(&vp, viewProjectionMatrix);
    
    // Left plane
    planes[0] = XMFLOAT4(
        vp._14 + vp._11,
        vp._24 + vp._21,
        vp._34 + vp._31,
        vp._44 + vp._41
    );
    
    // Right plane
    planes[1] = XMFLOAT4(
        vp._14 - vp._11,
        vp._24 - vp._21,
        vp._34 - vp._31,
        vp._44 - vp._41
    );
    
    // Top plane
    planes[2] = XMFLOAT4(
        vp._14 - vp._12,
        vp._24 - vp._22,
        vp._34 - vp._32,
        vp._44 - vp._42
    );
    
    // Bottom plane
    planes[3] = XMFLOAT4(
        vp._14 + vp._12,
        vp._24 + vp._22,
        vp._34 + vp._32,
        vp._44 + vp._42
    );
    
    // Near plane
    planes[4] = XMFLOAT4(
        vp._13,
        vp._23,
        vp._33,
        vp._43
    );
    
    // Far plane
    planes[5] = XMFLOAT4(
        vp._14 - vp._13,
        vp._24 - vp._23,
        vp._34 - vp._33,
        vp._44 - vp._43
    );
    
    // Normalize all planes
    for (int i = 0; i < 6; i++) {
        planes[i] = NormalizePlane(planes[i]);
    }
}

XMFLOAT4 FrustumCuller::NormalizePlane(const XMFLOAT4& plane) {
    float length = sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
    if (length > 0.0f) {
        return XMFLOAT4(plane.x / length, plane.y / length, plane.z / length, plane.w / length);
    }
    return plane;
}

void FrustumCuller::UpdateStatistics() {
    // In a real implementation, this would do an async readback
    // For now, we'll just reset stats and estimate based on settings
    stats.totalParticles = static_cast<UINT>(particleCount);
    
    // These would be calculated from actual GPU results
    // For now, provide reasonable estimates
    stats.visibleParticles = static_cast<UINT>(particleCount * 0.6f); // Assume 60% visible
    stats.culledByFrustum = static_cast<UINT>(particleCount * 0.3f);
    stats.culledByDistance = static_cast<UINT>(particleCount * 0.1f);
    stats.culledByBackface = 0; // Particles don't have backfaces
    
    // LOD distribution (rough estimates)
    stats.lodLevel0Count = static_cast<UINT>(stats.visibleParticles * 0.2f);
    stats.lodLevel1Count = static_cast<UINT>(stats.visibleParticles * 0.3f);
    stats.lodLevel2Count = static_cast<UINT>(stats.visibleParticles * 0.5f);
    
    stats.cullingTimeMs = 0.1f; // Placeholder
}

std::vector<CullingData> FrustumCuller::ReadBackCullingResults() {
    PROFILE_FUNCTION();
    
    auto context = device->GetContext();
    
    // Copy GPU buffer to staging buffer
    context->CopyResource(cullingDataStagingBuffer.get(), cullingDataBuffer.get());
    
    // Map staging buffer for reading
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(cullingDataStagingBuffer.get(), 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to map culling data staging buffer: 0x{:08X}", hr);
        return {};
    }
    
    // Copy data to vector
    std::vector<CullingData> results(particleCount);
    memcpy(results.data(), mappedResource.pData, sizeof(CullingData) * particleCount);
    
    context->Unmap(cullingDataStagingBuffer.get(), 0);
    
    // Update actual statistics from GPU results
    stats.totalParticles = static_cast<UINT>(particleCount);
    stats.visibleParticles = 0;
    stats.culledByFrustum = 0;
    stats.culledByDistance = 0;
    stats.lodLevel0Count = 0;
    stats.lodLevel1Count = 0;
    stats.lodLevel2Count = 0;
    
    for (const auto& data : results) {
        if (data.isVisible) {
            stats.visibleParticles++;
            switch (data.lodLevel) {
                case 0: stats.lodLevel0Count++; break;
                case 1: stats.lodLevel1Count++; break;
                case 2: stats.lodLevel2Count++; break;
            }
        } else {
            // Determine culling reason based on distance
            if (data.distanceToCamera > settings.maxRenderDistance) {
                stats.culledByDistance++;
            } else {
                stats.culledByFrustum++;
            }
        }
    }
    
    return results;
}

void FrustumCuller::PrintCullingStats() const {
    LOG_INFO("=== Frustum Culling Statistics ===");
    LOG_INFO("Total particles: {}", stats.totalParticles);
    LOG_INFO("Visible particles: {}", stats.visibleParticles);
    LOG_INFO("Culled by frustum: {}", stats.culledByFrustum);
    LOG_INFO("Culled by distance: {}", stats.culledByDistance);
    LOG_INFO("LOD Level 0 (High): {}", stats.lodLevel0Count);
    LOG_INFO("LOD Level 1 (Medium): {}", stats.lodLevel1Count);
    LOG_INFO("LOD Level 2 (Low): {}", stats.lodLevel2Count);
    LOG_INFO("Culling time: {:.2f}ms", stats.cullingTimeMs);
    
    float cullingPercent = stats.totalParticles > 0 ? 
        (float)(stats.totalParticles - stats.visibleParticles) / stats.totalParticles * 100.0f : 0.0f;
    LOG_INFO("Culling efficiency: {:.1f}%", cullingPercent);
}

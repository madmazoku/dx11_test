#include "Renderer.h"
#include "Utilities.h"
#include "Logger.h"
#include "Profiler.h"
#include <iostream>
#include <cmath>

using namespace DirectX;

Renderer::Renderer(std::shared_ptr<D3DDevice> device, 
                   std::shared_ptr<ShaderManager> shaderManager,
                   const RenderConfig& config)
    : device(device), shaderManager(shaderManager), config(config) {
    cameraPosition = { 0.0f, config.cameraHeight, config.cameraRadius };
}

bool Renderer::Initialize() {
    PROFILE_FUNCTION();
    
    try {
        if (!CreateConstantBuffers()) return false;
        if (!CreateRenderStates()) return false;
        
        LOG_INFO("Renderer initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Renderer initialization failed: {}", e.what());
        return false;
    }
}

bool Renderer::CreateConstantBuffers() {
    auto d3dDevice = device->GetDevice();
    
    // Create transform constant buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(TransformBuffer);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, transformBuffer.getAddressOf()),
        "Failed to create transform constant buffer"
    );

    // Create light constant buffer
    bufferDesc.ByteWidth = sizeof(LightBuffer);
    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, lightBuffer.getAddressOf()),
        "Failed to create light constant buffer"
    );

    // Create simulation constants buffer for compute shader
    bufferDesc.ByteWidth = sizeof(SimulationConstants);
    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, simulationConstantsBuffer.getAddressOf()),
        "Failed to create simulation constants buffer"
    );

    return true;
}

bool Renderer::CreateRenderStates() {
    auto d3dDevice = device->GetDevice();
    
    // Create rasterizer state
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.AntialiasedLineEnable = false;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.FrontCounterClockwise = false;
    rasterizerDesc.MultisampleEnable = false;
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    THROW_IF_FAILED(
        d3dDevice->CreateRasterizerState(&rasterizerDesc, rasterizerState.getAddressOf()),
        "Failed to create rasterizer state"
    );

    // Create depth stencil state
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = false;

    THROW_IF_FAILED(
        d3dDevice->CreateDepthStencilState(&depthStencilDesc, depthStencilState.getAddressOf()),
        "Failed to create depth stencil state"
    );

    // Create blend state for transparency (optional)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = false;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    THROW_IF_FAILED(
        d3dDevice->CreateBlendState(&blendDesc, blendState.getAddressOf()),
        "Failed to create blend state"
    );

    return true;
}

void Renderer::RenderFrame(const ParticleSystem& particleSystem, float deltaTime) {
    PROFILE_SCOPE("RenderFrame");
    
    auto context = device->GetContext();
    
    // Run compute shader first
    {
        PROFILE_SCOPE("Compute Shader");
        RunComputeShader(particleSystem);
    }
    
    // Update constant buffers
    {
        PROFILE_SCOPE("Update Buffers");
        UpdateTransformBuffer(deltaTime);
        UpdateLightBuffer();
        UpdateSimulationConstantsBuffer(particleSystem);
    }

    // Render particles
    {
        PROFILE_SCOPE("Draw Particles");
        
        // Set shaders
        context->VSSetShader(shaderManager->GetVertexShader("vertex"), nullptr, 0);
        context->GSSetShader(shaderManager->GetGeometryShader("geometry"), nullptr, 0);
        context->PSSetShader(shaderManager->GetPixelShader("pixel"), nullptr, 0);

        // Set constant buffers
        ID3D11Buffer* vsBuffers[] = { transformBuffer.get() };
        ID3D11Buffer* gsBuffers[] = { transformBuffer.get() };
        ID3D11Buffer* psBuffers[] = { lightBuffer.get() };
        
        context->VSSetConstantBuffers(0, 1, vsBuffers);
        context->GSSetConstantBuffers(0, 1, gsBuffers);
        context->PSSetConstantBuffers(0, 1, psBuffers);

        // Set shader resources - get the updated buffer from compute shader
        ID3D11ShaderResourceView* srvs[] = { particleSystem.GetCurrentSRV() };
        context->VSSetShaderResources(0, 1, srvs);

        // Set render states
        context->RSSetState(rasterizerState.get());
        context->OMSetDepthStencilState(depthStencilState.get(), 1);
        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->OMSetBlendState(blendState.get(), blendFactor, 0xffffffff);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Dispatch compute shader for physics
    auto computeShader = shaderManager->GetComputeShader("compute");
    if (computeShader) {
        context->CSSetShader(computeShader, nullptr, 0);
        
        ID3D11ShaderResourceView* csSRVs[] = { particleSystem.GetCurrentSRV() };
        ID3D11UnorderedAccessView* csUAVs[] = { particleSystem.GetNextUAV() };
        
        context->CSSetShaderResources(0, 1, csSRVs);
        context->CSSetUnorderedAccessViews(0, 1, csUAVs, nullptr);
        
        // Dispatch compute shader
        UINT numGroups = (Utils::SafeSizeTToUINT(particleSystem.GetParticleCount()) + 63) / 64;
        context->Dispatch(numGroups, 1, 1);
        
        // Unset compute shader resources
        ID3D11ShaderResourceView* nullSRV = nullptr;
        ID3D11UnorderedAccessView* nullUAV = nullptr;
        context->CSSetShaderResources(0, 1, &nullSRV);
        context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
        context->CSSetShader(nullptr, nullptr, 0);
    }

    // Draw particles (will be expanded to spheres by geometry shader)
    context->Draw(Utils::SafeSizeTToUINT(particleSystem.GetParticleCount()), 0);

    // Unset resources
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->VSSetShaderResources(0, 1, &nullSRV);
    context->VSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
}

void Renderer::RenderFrameWithCamera(const ParticleSystem& particleSystem, 
                                    const InteractiveCamera& camera, float deltaTime) {
    PROFILE_SCOPE("RenderFrameWithCamera");
    
    auto context = device->GetContext();
    
    // Run compute shader first
    {
        PROFILE_SCOPE("Compute Shader");
        RunComputeShader(particleSystem);
    }
    
    // Update constant buffers with camera data
    {
        PROFILE_SCOPE("Update Buffers");
        UpdateTransformBufferWithCamera(camera, deltaTime);
        UpdateLightBufferWithCamera(camera);
        UpdateSimulationConstantsBuffer(particleSystem);
    }

    // Render particles
    {
        PROFILE_SCOPE("Draw Particles");
        
        // Set shaders
        context->VSSetShader(shaderManager->GetVertexShader("vertex"), nullptr, 0);
        context->GSSetShader(shaderManager->GetGeometryShader("geometry"), nullptr, 0);
        context->PSSetShader(shaderManager->GetPixelShader("pixel"), nullptr, 0);

        // Set constant buffers
        ID3D11Buffer* vsBuffers[] = { transformBuffer.get() };
        ID3D11Buffer* gsBuffers[] = { transformBuffer.get() };
        ID3D11Buffer* psBuffers[] = { lightBuffer.get() };
        
        context->VSSetConstantBuffers(0, 1, vsBuffers);
        context->GSSetConstantBuffers(0, 1, gsBuffers);
        context->PSSetConstantBuffers(0, 1, psBuffers);

        // Set shader resources
        ID3D11ShaderResourceView* srvs[] = { particleSystem.GetCurrentSRV() };
        context->VSSetShaderResources(0, 1, srvs);

        // Set render states
        context->RSSetState(rasterizerState.get());
        context->OMSetDepthStencilState(depthStencilState.get(), 1);
        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        context->OMSetBlendState(blendState.get(), blendFactor, 0xffffffff);

        // Set primitive topology
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

        // Draw particles
        context->Draw(Utils::SafeSizeTToUINT(particleSystem.GetParticleCount()), 0);

        // Unset resources
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->VSSetShaderResources(0, 1, &nullSRV);
        context->VSSetShader(nullptr, nullptr, 0);
        context->GSSetShader(nullptr, nullptr, 0);
        context->PSSetShader(nullptr, nullptr, 0);
    }
}

void Renderer::UpdateTransformBuffer(float time) {
    UpdateCameraPosition(time);
    
    XMMATRIX worldMatrix = XMMatrixIdentity();
    XMMATRIX viewMatrix = CreateViewMatrix();
    XMMATRIX projMatrix = CreateProjectionMatrix();
    XMMATRIX viewProjMatrix = XMMatrixMultiply(viewMatrix, projMatrix);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(transformBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map transform buffer"
    );

    TransformBuffer* transformData = static_cast<TransformBuffer*>(mappedResource.pData);
    transformData->viewProjectionMatrix = XMMatrixTranspose(viewProjMatrix);
    transformData->worldMatrix = XMMatrixTranspose(worldMatrix);
    transformData->cameraPos = cameraPosition;
    transformData->sphereRadius = config.sphereRadius;

    device->GetContext()->Unmap(transformBuffer.get(), 0);
}

void Renderer::UpdateTransformBufferWithCamera(const InteractiveCamera& camera, float time) {
    XMMATRIX worldMatrix = XMMatrixIdentity();
    
    float aspectRatio = static_cast<float>(config.windowWidth) / static_cast<float>(config.windowHeight);
    XMMATRIX viewMatrix = camera.GetViewMatrix();
    XMMATRIX projMatrix = camera.GetProjectionMatrix(aspectRatio);
    XMMATRIX viewProjMatrix = XMMatrixMultiply(viewMatrix, projMatrix);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(transformBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map transform buffer"
    );

    TransformBuffer* transformData = static_cast<TransformBuffer*>(mappedResource.pData);
    transformData->viewProjectionMatrix = XMMatrixTranspose(viewProjMatrix);
    transformData->worldMatrix = XMMatrixTranspose(worldMatrix);
    
    XMFLOAT3 cameraPos = camera.GetPosition();
    transformData->cameraPos = cameraPos;
    transformData->sphereRadius = config.sphereRadius;

    device->GetContext()->Unmap(transformBuffer.get(), 0);
}

void Renderer::UpdateLightBuffer() {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(lightBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map light buffer"
    );

    LightBuffer* lightData = static_cast<LightBuffer*>(mappedResource.pData);
    lightData->lightDirection = XMFLOAT3(config.lightDirection[0], config.lightDirection[1], config.lightDirection[2]);
    lightData->lightIntensity = config.lightIntensity;
    lightData->lightColor = XMFLOAT3(config.lightColor[0], config.lightColor[1], config.lightColor[2]);
    lightData->ambientIntensity = config.ambientIntensity;
    lightData->cameraPos = cameraPosition;
    lightData->specularPower = config.specularPower;

    device->GetContext()->Unmap(lightBuffer.get(), 0);
}

void Renderer::UpdateLightBufferWithCamera(const InteractiveCamera& camera) {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(lightBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map light buffer"
    );

    LightBuffer* lightData = static_cast<LightBuffer*>(mappedResource.pData);
    lightData->lightDirection = XMFLOAT3(config.lightDirection[0], config.lightDirection[1], config.lightDirection[2]);
    lightData->lightIntensity = config.lightIntensity;
    lightData->lightColor = XMFLOAT3(config.lightColor[0], config.lightColor[1], config.lightColor[2]);
    lightData->ambientIntensity = config.ambientIntensity;
    lightData->cameraPos = camera.GetPosition();
    lightData->specularPower = config.specularPower;

    device->GetContext()->Unmap(lightBuffer.get(), 0);
}

void Renderer::UpdateSimulationConstantsBuffer(const ParticleSystem& particleSystem) {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(simulationConstantsBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map simulation constants buffer"
    );

    SimulationConstants* simData = static_cast<SimulationConstants*>(mappedResource.pData);
    const auto& config = particleSystem.GetConfig();
    
    simData->springConstant = config.springConstant;
    simData->restLength = config.restLength;
    simData->timeStep = config.timeStep;
    simData->damping = config.damping;
    simData->gravity = XMFLOAT3(config.gravity[0], config.gravity[1], config.gravity[2]);
    simData->boundaryMin = XMFLOAT3(config.boundaryMin[0], config.boundaryMin[1], config.boundaryMin[2]);
    simData->boundaryMax = XMFLOAT3(config.boundaryMax[0], config.boundaryMax[1], config.boundaryMax[2]);

    device->GetContext()->Unmap(simulationConstantsBuffer.get(), 0);
}

void Renderer::UpdateCameraPosition(float time) {
    cameraAngle = time * config.cameraRotationSpeed;
    cameraDistance = config.cameraRadius;
    cameraPosition.x = cameraDistance * sinf(cameraAngle);
    cameraPosition.y = config.cameraHeight;
    cameraPosition.z = cameraDistance * cosf(cameraAngle);
}

XMMATRIX Renderer::CreateViewMatrix() const {
    XMVECTOR eye = XMLoadFloat3(&cameraPosition);
    XMVECTOR at = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    return XMMatrixLookAtLH(eye, at, up);
}

XMMATRIX Renderer::CreateProjectionMatrix() const {
    float fovY = XMConvertToRadians(config.cameraFov);
    float aspectRatio = static_cast<float>(config.windowWidth) / static_cast<float>(config.windowHeight);
    float nearZ = config.cameraNearPlane;
    float farZ = config.cameraFarPlane;
    return XMMatrixPerspectiveFovLH(fovY, aspectRatio, nearZ, farZ);
}

void Renderer::RunComputeShader(const ParticleSystem& particleSystem) {
    PROFILE_SCOPE("Compute Shader");
    
    auto context = device->GetContext();
    
    // Set compute shader
    context->CSSetShader(shaderManager->GetComputeShader("compute"), nullptr, 0);
    
    // Set resources
    ID3D11ShaderResourceView* srvs[] = { particleSystem.GetCurrentSRV() };
    ID3D11UnorderedAccessView* uavs[] = { particleSystem.GetNextUAV() };
    
    context->CSSetShaderResources(0, 1, srvs);
    context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
    
    // Set simulation constants constant buffer
    if (simulationConstantsBuffer) {
        ID3D11Buffer* buffers[] = { simulationConstantsBuffer.get() };
        context->CSSetConstantBuffers(0, 1, buffers);
    }
    
    // Dispatch compute shader
    UINT numGroups = (static_cast<UINT>(particleSystem.GetParticleCount()) + 63) / 64;
    context->Dispatch(numGroups, 1, 1);
    
    // Unbind resources
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ID3D11UnorderedAccessView* nullUAV = nullptr;
    context->CSSetShaderResources(0, 1, &nullSRV);
    context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);
}

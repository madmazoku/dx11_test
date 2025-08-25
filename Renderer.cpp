#include "Renderer.h"
#include "Utilities.h"
#include "Logger.h"
#include "Profiler.h"
#include <iostream>
#include <cmath>

using namespace DirectX;

Renderer::Renderer(std::shared_ptr<D3DDevice> device, 
                   std::shared_ptr<ShaderManager> shaderManager,
                   std::shared_ptr<ConfigManager> configManager)
    : device(device), shaderManager(shaderManager), configManager(configManager) {
    
    // Get configuration
    const auto& renderConfig = configManager->GetRenderConfig();
    const auto& env = configManager->GetEnvironment();
    
    cameraPosition = { 0.0f, renderConfig.cameraHeight, renderConfig.cameraRadius };
    
    // Initialize material buffer array
    materials.clear();
    for (const auto& particleType : configManager->GetParticleTypes()) {
        materials.push_back(particleType.material);
    }
}

bool Renderer::Initialize() {
    return InitializeWithMaxParticles(100000); // Default max particles
}

bool Renderer::InitializeWithMaxParticles(size_t maxParticles) {
    PROFILE_FUNCTION();
    
    try {
        if (!CreateConstantBuffers()) return false;
        if (!CreateRenderStates()) return false;
        if (!CreateMaterialBuffer()) return false;
        
        // Initialize frustum culler with multi-type support
        frustumCuller = std::make_unique<FrustumCuller>(device, shaderManager);
        if (!frustumCuller->Initialize(maxParticles)) {
            LOG_WARNING("Failed to initialize frustum culler - continuing without culling");
            frustumCuller.reset();
            enableFrustumCulling = false;
        } else {
            const auto& cullingConfig = configManager->GetCullingConfig();
            frustumCuller->SetMaxRenderDistance(cullingConfig.maxRenderDistance);
            frustumCuller->SetLODDistances(cullingConfig.lodDistance1, 
                                         cullingConfig.lodDistance2, 
                                         cullingConfig.lodDistance3);
            frustumCuller->EnableLOD(cullingConfig.enableLOD);
            frustumCuller->EnableBackfaceCulling(cullingConfig.enableBackfaceCulling);
            frustumCuller->EnableDistanceCulling(cullingConfig.enableDistanceCulling);
            enableFrustumCulling = cullingConfig.enableFrustumCulling;
        }
        
        LOG_INFO("Multi-type renderer initialized successfully with {} particle types, frustum culling {}", 
                configManager->GetParticleTypeCount(),
                frustumCuller ? "enabled" : "disabled");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Multi-type renderer initialization failed: {}", e.what());
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

    // Create simulation constants buffer for multi-type compute shader
    bufferDesc.ByteWidth = sizeof(SimulationConstants);
    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, simulationConstantsBuffer.getAddressOf()),
        "Failed to create simulation constants buffer"
    );

    return true;
}

bool Renderer::CreateMaterialBuffer() {
    auto d3dDevice = device->GetDevice();
    
    if (materials.empty()) {
        LOG_ERROR("No materials loaded from configuration");
        return false;
    }
    
    // Create structured buffer for materials
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(MaterialProperties) * materials.size();
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.StructureByteStride = sizeof(MaterialProperties);
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = materials.data();

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, &initData, materialBuffer.getAddressOf()),
        "Failed to create material buffer"
    );

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = materials.size();

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(materialBuffer.get(), &srvDesc, materialBufferSRV.getAddressOf()),
        "Failed to create material buffer SRV"
    );

    LOG_INFO("Created material buffer with {} materials", materials.size());
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

    // Create blend state for transparent particles
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = true;
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

void Renderer::UpdateSimulationConstants(const std::vector<Particle>& particles) {
    PROFILE_FUNCTION();
    
    auto context = device->GetContext();
    const auto& env = configManager->GetEnvironment();
    const auto& simConfig = configManager->GetSimulationConfig();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        context->Map(simulationConstantsBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map simulation constants buffer"
    );

    auto constants = static_cast<SimulationConstants*>(mappedResource.pData);
    
    constants->deltaTime = simConfig.timeStep;
    constants->globalDamping = 0.999f; // Global damping factor
    
    constants->gravity = env.gravity;
    constants->boundaryRestitution = env.boundaryRestitution;
    
    constants->boundaryMin = env.boundaryMin;
    constants->airDensity = env.airDensity;
    
    constants->boundaryMax = env.boundaryMax;
    constants->fluidViscosity = env.fluidViscosity;
    
    constants->windVelocity = env.windVelocity;
    constants->ambientTemperature = env.ambientTemperature;
    
    constants->numParticleTypes = configManager->GetParticleTypeCount();
    constants->numInteractionRules = configManager->GetInteractionRuleCount();
    constants->maxInteractionsPerParticle = simConfig.maxInteractionsPerParticle;
    constants->spatialGridSize = simConfig.spatialGridSize;
    
    constants->thermalDiffusion = env.thermalDiffusion;

    context->Unmap(simulationConstantsBuffer.get(), 0);
}

void Renderer::RenderFrameWithCamera(const std::vector<Particle>& particles, 
                                   const XMMATRIX& viewMatrix, 
                                   const XMMATRIX& projectionMatrix,
                                   const XMFLOAT3& cameraPos) {
    PROFILE_FUNCTION();
    
    auto context = device->GetContext();
    const auto& renderConfig = configManager->GetRenderConfig();
    
    // Clear render targets
    const float clearColor[] = { 
        renderConfig.clearColor[0], 
        renderConfig.clearColor[1], 
        renderConfig.clearColor[2], 
        renderConfig.clearColor[3] 
    };
    context->ClearRenderTargetView(device->GetRenderTargetView(), clearColor);
    context->ClearDepthStencilView(device->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Set render states
    context->RSSetState(rasterizerState.get());
    context->OMSetDepthStencilState(depthStencilState.get(), 1);
    
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->OMSetBlendState(blendState.get(), blendFactor, 0xffffffff);

    // Apply frustum culling if enabled
    std::vector<Particle> visibleParticles = particles;
    if (enableFrustumCulling && frustumCuller) {
        XMMATRIX viewProjMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
        visibleParticles = frustumCuller->CullParticles(particles, viewProjMatrix, cameraPos);
        
        LOG_DEBUG("Culled {} particles, {} remain visible", 
                 particles.size() - visibleParticles.size(), 
                 visibleParticles.size());
    }

    if (visibleParticles.empty()) {
        return;
    }

    // Update transform buffer
    UpdateTransformBuffer(viewMatrix, projectionMatrix, cameraPos);
    
    // Update lighting buffer
    UpdateLightBuffer(cameraPos);
    
    // Set shaders
    if (!shaderManager->SetShaders("VertexShader", "GeometryShaderIcosphere", "PixelShaderPhong")) {
        LOG_ERROR("Failed to set multi-type rendering shaders");
        return;
    }

    // Bind constant buffers
    context->VSSetConstantBuffers(0, 1, transformBuffer.getAddressOf());
    context->GSSetConstantBuffers(0, 1, transformBuffer.getAddressOf());
    context->GSSetConstantBuffers(1, 1, GetCullingBuffer()); // LOD/culling constants
    context->PSSetConstantBuffers(0, 1, lightBuffer.getAddressOf());

    // Bind particle data and material buffer
    auto particleBuffer = shaderManager->GetParticleBuffer();
    auto particleSRV = shaderManager->GetParticleBufferSRV();
    
    if (particleBuffer && particleSRV) {
        context->VSSetShaderResources(0, 1, &particleSRV);
        context->PSSetShaderResources(0, 1, materialBufferSRV.getAddressOf());
    }

    // Set input layout and topology
    context->IASetInputLayout(nullptr); // No input layout needed for structured buffer
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Render particles as points (geometry shader generates icospheres)
    context->Draw(visibleParticles.size(), 0);

    LOG_DEBUG("Rendered {} particles as icospheres with Phong shading", visibleParticles.size());
}

void Renderer::UpdateTransformBuffer(const XMMATRIX& viewMatrix, 
                                   const XMMATRIX& projectionMatrix, 
                                   const XMFLOAT3& cameraPos) {
    PROFILE_FUNCTION();
    
    auto context = device->GetContext();
    const auto& cullingConfig = configManager->GetCullingConfig();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        context->Map(transformBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map transform buffer"
    );

    auto transformData = static_cast<TransformBuffer*>(mappedResource.pData);
    
    transformData->viewProjectionMatrix = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix));
    transformData->worldMatrix = XMMatrixTranspose(XMMatrixIdentity());
    transformData->cameraPos = cameraPos;
    transformData->globalScale = cullingConfig.globalScale;

    context->Unmap(transformBuffer.get(), 0);
}

void Renderer::UpdateLightBuffer(const XMFLOAT3& cameraPos) {
    PROFILE_FUNCTION();
    
    auto context = device->GetContext();
    const auto& renderConfig = configManager->GetRenderConfig();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        context->Map(lightBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map light buffer"
    );

    auto lightData = static_cast<LightBuffer*>(mappedResource.pData);
    
    lightData->lightDirection = { 
        renderConfig.lightDirection[0], 
        renderConfig.lightDirection[1], 
        renderConfig.lightDirection[2] 
    };
    lightData->lightIntensity = renderConfig.lightIntensity;
    lightData->lightColor = { 
        renderConfig.lightColor[0], 
        renderConfig.lightColor[1], 
        renderConfig.lightColor[2] 
    };
    lightData->ambientIntensity = renderConfig.ambientIntensity;
    lightData->cameraPos = cameraPos;
    lightData->specularPower = renderConfig.specularPower;

    context->Unmap(lightBuffer.get(), 0);
}

ID3D11Buffer** Renderer::GetCullingBuffer() {
    // Create culling/LOD buffer if needed
    if (!cullingBuffer) {
        CreateCullingBuffer();
    }
    return cullingBuffer.getAddressOf();
}

bool Renderer::CreateCullingBuffer() {
    auto d3dDevice = device->GetDevice();
    const auto& icoConfig = configManager->GetIcosphereConfig();
    
    // Structure for culling/LOD constants
    struct CullingConstants {
        float lodDistance0;
        float lodDistance1;
        float lodDistance2;
        float maxRenderDistance;
        uint32_t enableLOD;
        uint32_t enableDistanceCulling;
        float padding[2];
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(CullingConstants);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    CullingConstants constants = {};
    constants.lodDistance0 = icoConfig.lodDistance0;
    constants.lodDistance1 = icoConfig.lodDistance1;
    constants.lodDistance2 = icoConfig.lodDistance2;
    constants.maxRenderDistance = configManager->GetCullingConfig().maxRenderDistance;
    constants.enableLOD = icoConfig.enableAdaptiveLOD ? 1 : 0;
    constants.enableDistanceCulling = configManager->GetCullingConfig().enableDistanceCulling ? 1 : 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &constants;

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, &initData, cullingBuffer.getAddressOf()),
        "Failed to create culling constant buffer"
    );

    return true;
}

void Renderer::EnableFrustumCulling(bool enable) {
    enableFrustumCulling = enable && (frustumCuller != nullptr);
    LOG_INFO("Frustum culling {}", enableFrustumCulling ? "enabled" : "disabled");
}

void Renderer::SetCameraPosition(const XMFLOAT3& position) {
    cameraPosition = position;
}

XMFLOAT3 Renderer::GetCameraPosition() const {
    return cameraPosition;
}

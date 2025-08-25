/**
 * @file ParticleSystem.cpp
 * @brief Multi-type particle physics simulation system
 * 
 * This file implements a comprehensive GPU-accelerated particle physics system
 * supporting multiple particle types with force-based interactions. Key features:
 * 
 * - Multi-type particles with distinct physical properties (mass, radius, charge)
 * - Force-based interactions: spring, Lennard-Jones, electromagnetic, gravitational
 * - GPU compute shader simulation for high-performance parallel processing
 * - DirectX 11 buffer management for efficient GPU data transfer
 * - Configurable particle distributions and initial conditions
 * - Real-time parameter updates and simulation control
 * 
 * The system can simulate hundreds to thousands of particles in real-time
 * while maintaining stable physics and interactive frame rates.
 */

#include "ParticleSystem.h"
#include "Utilities.h"
#include "Logger.h"
#include "Profiler.h"
#include <random>
#include <algorithm>
#include <iostream>

/**
 * Constructor - Initialize particle system with device and configuration
 * 
 * Sets up the particle system with the specified number of particles and
 * prepares data structures for multi-type simulation. The actual GPU
 * buffers and compute resources are created during Initialize().
 * 
 * @param device         DirectX 11 device for GPU resource creation
 * @param configManager  Configuration manager containing simulation parameters
 */
ParticleSystem::ParticleSystem(std::shared_ptr<D3DDevice> device, 
                               std::shared_ptr<ConfigManager> configManager)
    : device(device), configManager(configManager) {
    
    // Allocate particle storage based on configuration
    const auto& simConfig = configManager->GetSimulationConfig();
    particles.resize(simConfig.particleCount);
    
    // Prepare particle type definitions and interaction rules for GPU upload
    InitializeTypeAndRuleData();
}

bool ParticleSystem::Initialize() {
    try {
        InitializeParticles();
        if (!CreateParticleBuffers()) return false;
        if (!CreateTypeBuffers()) return false;
        if (!CreateRuleBuffers()) return false;
        if (!CreateConstantBuffer()) return false;
        UpdateConstantBuffer();
        
        LOG_INFO("Multi-type particle system initialized: {} particles, {} types, {} rules",
                particles.size(), configManager->GetParticleTypeCount(), 
                configManager->GetInteractionRuleCount());
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Multi-type particle system initialization failed: {}", e.what());
        return false;
    }
}

void ParticleSystem::InitializeTypeAndRuleData() {
    // Convert config structures to GPU-friendly formats
    const auto& particleTypes = configManager->GetParticleTypes();
    const auto& interactionRules = configManager->GetInteractionRules();
    
    // Create GPU particle type info
    gpuParticleTypes.clear();
    for (const auto& type : particleTypes) {
        GPUParticleType gpuType = {};
        gpuType.id = type.id;
        gpuType.mass = type.mass;
        gpuType.radius = type.radius;
        gpuType.density = type.density;
        gpuType.charge = type.charge;
        gpuType.temperature = type.temperature;
        gpuType.restitution = type.restitution;
        gpuType.staticFriction = type.staticFriction;
        gpuType.dynamicFriction = type.dynamicFriction;
        gpuType.enableCollisions = type.enableCollisions ? 1 : 0;
        gpuType.airResistance = type.airResistance;
        gpuType.thermalConductivity = type.thermalConductivity;
        
        gpuParticleTypes.push_back(gpuType);
    }
    
    // Create GPU interaction rules
    gpuInteractionRules.clear();
    for (const auto& rule : interactionRules) {
        GPUInteractionRule gpuRule = {};
        gpuRule.typeA = rule.typeA;
        gpuRule.typeB = rule.typeB;
        gpuRule.enabled = rule.enabled ? 1 : 0;
        gpuRule.temperatureInfluence = rule.temperatureInfluence;
        gpuRule.velocityInfluence = rule.velocityInfluence;
        
        // Convert force parameters
        gpuRule.forceType = static_cast<uint32_t>(rule.force.type);
        gpuRule.forceStrength = rule.force.strength;
        gpuRule.forceRange = rule.force.range;
        gpuRule.optimalDistance = rule.force.optimalDistance;
        
        // Pack type-specific parameters
        switch (rule.force.type) {
            case ForceType::Spring:
                gpuRule.forceParams[0] = rule.force.spring.stiffness;
                gpuRule.forceParams[1] = rule.force.spring.dampingCoeff;
                break;
            case ForceType::LennardJones:
                gpuRule.forceParams[0] = rule.force.lennardJones.epsilon;
                gpuRule.forceParams[1] = rule.force.lennardJones.sigma;
                break;
            case ForceType::Gravitational:
                gpuRule.forceParams[0] = rule.force.gravitational.gravitationalConstant;
                break;
            case ForceType::Electromagnetic:
                gpuRule.forceParams[0] = rule.force.electromagnetic.coulombConstant;
                break;
            case ForceType::Viscous:
                gpuRule.forceParams[0] = rule.force.viscous.viscosity;
                gpuRule.forceParams[1] = rule.force.viscous.dragCoefficient;
                break;
        }
        
        gpuInteractionRules.push_back(gpuRule);
    }
}

void ParticleSystem::InitializeParticles() {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    const auto& typeDistribution = configManager->GetTypeDistribution();
    const auto& particleTypes = configManager->GetParticleTypes();
    
    size_t particleIndex = 0;
    
    // Distribute particles according to configuration
    for (const auto& dist : typeDistribution) {
        size_t numParticlesForType = static_cast<size_t>(particles.size() * dist.percentage);
        
        // Find the particle type
        auto typeIt = std::find_if(particleTypes.begin(), particleTypes.end(),
            [&](const ParticleTypeInfo& type) { return type.id == dist.typeId; });
        
        if (typeIt == particleTypes.end()) {
            LOG_WARNING("Particle type {} not found, skipping distribution", dist.typeId);
            continue;
        }
        
        // Generate random positions in spawn area
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
        std::uniform_real_distribution<float> radiusDist(0.0f, dist.spawnRadius);
        std::uniform_real_distribution<float> heightDist(-dist.spawnRadius * 0.5f, dist.spawnRadius * 0.5f);
        
        for (size_t i = 0; i < numParticlesForType && particleIndex < particles.size(); i++, particleIndex++) {
            auto& particle = particles[particleIndex];
            
            // Random position within spawn sphere
            float angle = angleDist(gen);
            float radius = radiusDist(gen);
            float height = heightDist(gen);
            
            particle.position.x = dist.spawnCenter.x + radius * cos(angle);
            particle.position.y = dist.spawnCenter.y + height;
            particle.position.z = dist.spawnCenter.z + radius * sin(angle);
            
            // Initial velocity
            particle.velocity = dist.initialVelocity;
            particle.oldPosition = particle.position; // For Verlet integration
            
            // Type and properties
            particle.typeId = dist.typeId;
            particle.mass = typeIt->mass;
            particle.radius = typeIt->radius;
            particle.charge = typeIt->charge;
            particle.temperature = typeIt->temperature;
            
            // Initialize state
            particle.acceleration = { 0.0f, 0.0f, 0.0f };
            particle.age = 0.0f;
            particle.collisionCount = 0;
            particle.flags = typeIt->enableCollisions ? 1 : 0;
        }
    }
    
    // Fill remaining particles with default type if needed
    if (particleIndex < particles.size() && !particleTypes.empty()) {
        const auto& defaultType = particleTypes[0];
        std::uniform_real_distribution<float> posDist(-5.0f, 5.0f);
        
        for (size_t i = particleIndex; i < particles.size(); i++) {
            auto& particle = particles[i];
            
            particle.position = { posDist(gen), posDist(gen), posDist(gen) };
            particle.velocity = { 0.0f, 0.0f, 0.0f };
            particle.oldPosition = particle.position;
            particle.acceleration = { 0.0f, 0.0f, 0.0f };
            
            particle.typeId = defaultType.id;
            particle.mass = defaultType.mass;
            particle.radius = defaultType.radius;
            particle.charge = defaultType.charge;
            particle.temperature = defaultType.temperature;
            particle.age = 0.0f;
            particle.collisionCount = 0;
            particle.flags = defaultType.enableCollisions ? 1 : 0;
        }
    }
    
    LOG_INFO("Initialized {} particles with type distribution", particles.size());
}

bool ParticleSystem::CreateParticleBuffers() {
    auto d3dDevice = device->GetDevice();
    
    // Create structured buffers for particle data
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(Particle) * particles.size();
    bufferDesc.StructureByteStride = sizeof(Particle);
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = particles.data();

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, &initData, particleBufferA.getAddressOf()),
        "Failed to create particle buffer A"
    );

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, &initData, particleBufferB.getAddressOf()),
        "Failed to create particle buffer B"
    );

    // Create Shader Resource Views
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = particles.size();
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(particleBufferA.get(), &srvDesc, particleSRVA.getAddressOf()),
        "Failed to create particle SRV A"
    );

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(particleBufferB.get(), &srvDesc, particleSRVB.getAddressOf()),
        "Failed to create particle SRV B"
    );

    // Create Unordered Access Views
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = particles.size();
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;

    THROW_IF_FAILED(
        d3dDevice->CreateUnorderedAccessView(particleBufferA.get(), &uavDesc, particleUAVA.getAddressOf()),
        "Failed to create particle UAV A"
    );

    THROW_IF_FAILED(
        d3dDevice->CreateUnorderedAccessView(particleBufferB.get(), &uavDesc, particleUAVB.getAddressOf()),
        "Failed to create particle UAV B"
    );

    return true;
}

bool ParticleSystem::CreateTypeBuffers() {
    if (gpuParticleTypes.empty()) return true;
    
    auto d3dDevice = device->GetDevice();
    
    // Create particle type info buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(GPUParticleType) * gpuParticleTypes.size();
    bufferDesc.StructureByteStride = sizeof(GPUParticleType);
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = gpuParticleTypes.data();

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, &initData, particleTypeBuffer.getAddressOf()),
        "Failed to create particle type buffer"
    );

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = gpuParticleTypes.size();
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(particleTypeBuffer.get(), &srvDesc, particleTypeSRV.getAddressOf()),
        "Failed to create particle type SRV"
    );

    return true;
}

bool ParticleSystem::CreateRuleBuffers() {
    if (gpuInteractionRules.empty()) return true;
    
    auto d3dDevice = device->GetDevice();
    
    // Create interaction rule buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(GPUInteractionRule) * gpuInteractionRules.size();
    bufferDesc.StructureByteStride = sizeof(GPUInteractionRule);
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = gpuInteractionRules.data();

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, &initData, interactionRuleBuffer.getAddressOf()),
        "Failed to create interaction rule buffer"
    );

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = gpuInteractionRules.size();
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(interactionRuleBuffer.get(), &srvDesc, interactionRuleSRV.getAddressOf()),
        "Failed to create interaction rule SRV"
    );

    return true;
}

bool ParticleSystem::CreateConstantBuffer() {
    auto d3dDevice = device->GetDevice();
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(SimulationConstants);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    THROW_IF_FAILED(
        d3dDevice->CreateBuffer(&bufferDesc, nullptr, constantBuffer.getAddressOf()),
        "Failed to create constant buffer"
    );

    return true;
}

void ParticleSystem::UpdateConstantBuffer() {
    PROFILE_FUNCTION();
    
    auto context = device->GetContext();
    const auto& env = configManager->GetEnvironment();
    const auto& simConfig = configManager->GetSimulationConfig();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        context->Map(constantBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map constant buffer"
    );

    auto constants = static_cast<SimulationConstants*>(mappedResource.pData);
    
    constants->deltaTime = simConfig.timeStep;
    constants->globalDamping = 0.995f; // Global damping factor
    
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

    context->Unmap(constantBuffer.get(), 0);
}

void ParticleSystem::Update(std::shared_ptr<ShaderManager> shaderManager) {
    PROFILE_FUNCTION();
    
    if (!shaderManager->SetComputeShader("ComputeShader")) {
        LOG_ERROR("Failed to set multi-type compute shader");
        return;
    }

    auto context = device->GetContext();
    
    // Update constant buffer
    UpdateConstantBuffer();

    // Set constant buffer
    context->CSSetConstantBuffers(0, 1, constantBuffer.getAddressOf());

    // Set input/output resources
    auto inputSRV = useBufferA ? particleSRVA.get() : particleSRVB.get();
    auto outputUAV = useBufferA ? particleUAVB.get() : particleUAVA.get();
    
    ID3D11ShaderResourceView* srvs[] = { inputSRV, particleTypeSRV.get(), interactionRuleSRV.get() };
    context->CSSetShaderResources(0, 3, srvs);
    context->CSSetUnorderedAccessViews(0, 1, &outputUAV, nullptr);

    // Dispatch compute shader
    UINT numGroups = (particles.size() + 63) / 64; // 64 threads per group
    context->Dispatch(numGroups, 1, 1);

    // Unbind resources
    ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr, nullptr };
    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    context->CSSetShaderResources(0, 3, nullSRV);
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);

    // Swap buffers
    useBufferA = !useBufferA;
}

const std::vector<Particle>& ParticleSystem::GetParticles() const {
    return particles;
}

ID3D11Buffer* ParticleSystem::GetCurrentParticleBuffer() const {
    return useBufferA ? particleBufferA.get() : particleBufferB.get();
}

ID3D11ShaderResourceView* ParticleSystem::GetCurrentParticleBufferSRV() const {
    return useBufferA ? particleSRVA.get() : particleSRVB.get();
}

void ParticleSystem::ReadbackParticleData() {
    // For debugging - copy particle data back from GPU
    PROFILE_FUNCTION();
    
    // Implementation would copy data from GPU buffer back to particles vector
    // This is expensive and should only be used for debugging
    LOG_DEBUG("Particle data readback not implemented in release build");
}

#include "ParticleSystem.h"
#include "Utilities.h"
#include "Logger.h"
#include "Profiler.h"
#include <random>
#include <iostream>

ParticleSystem::ParticleSystem(std::shared_ptr<D3DDevice> device, const SimulationConfig& config)
    : device(device), config(config) {
    particles.resize(config.particleCount);
}

bool ParticleSystem::Initialize() {
    try {
        InitializeParticles();
        if (!CreateBuffers()) return false;
        if (!CreateConstantBuffer()) return false;
        UpdateConstantBuffer();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "ParticleSystem initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void ParticleSystem::InitializeParticles() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-2.0f, 2.0f);
    
    for (auto& particle : particles) {
        // Random initial positions
        particle.position.x = posDist(gen);
        particle.position.y = posDist(gen);
        particle.position.z = posDist(gen);
        
        // Set old position same as current (no initial velocity for Verlet)
        particle.oldPosition = particle.position;
        
        // Zero initial acceleration
        particle.acceleration = { 0.0f, 0.0f, 0.0f };
        particle.padding = 0.0f;
    }
}

bool ParticleSystem::CreateBuffers() {
    auto d3dDevice = device->GetDevice();
    
    // Create structured buffers for particle data
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = Utils::SafeSizeTToUINT(sizeof(Particle) * particles.size());
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
    srvDesc.Buffer.NumElements = Utils::SafeSizeTToUINT(particles.size());
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(particleBufferA.get(), &srvDesc, particleSRVA.getAddressOf()),
        "Failed to create SRV A"
    );

    THROW_IF_FAILED(
        d3dDevice->CreateShaderResourceView(particleBufferB.get(), &srvDesc, particleSRVB.getAddressOf()),
        "Failed to create SRV B"
    );

    // Create Unordered Access Views
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = Utils::SafeSizeTToUINT(particles.size());
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;

    THROW_IF_FAILED(
        d3dDevice->CreateUnorderedAccessView(particleBufferA.get(), &uavDesc, particleUAVA.getAddressOf()),
        "Failed to create UAV A"
    );

    THROW_IF_FAILED(
        d3dDevice->CreateUnorderedAccessView(particleBufferB.get(), &uavDesc, particleUAVB.getAddressOf()),
        "Failed to create UAV B"
    );

    return true;
}

bool ParticleSystem::CreateConstantBuffer() {
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(SimulationConstants);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    THROW_IF_FAILED(
        device->GetDevice()->CreateBuffer(&bufferDesc, nullptr, simulationConstantsBuffer.getAddressOf()),
        "Failed to create simulation constants buffer"
    );

    return true;
}

void ParticleSystem::UpdateConstantBuffer() {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(simulationConstantsBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map simulation constants buffer"
    );

    SimulationConstants* constants = static_cast<SimulationConstants*>(mappedResource.pData);
    constants->springConstant = config.springConstant;
    constants->restLength = config.restLength;
    constants->timeStep = config.timeStep;
    constants->damping = config.damping;
    constants->gravity = config.gravity;
    constants->boundaryMin = config.boundaryMin;
    constants->boundaryMax = config.boundaryMax;

    device->GetContext()->Unmap(simulationConstantsBuffer.get(), 0);
}

void ParticleSystem::Update() {
    PROFILE_SCOPE("ParticleSystem::Update");
    
    // The actual compute shader dispatch happens in the Renderer
    SwapBuffers();
}

void ParticleSystem::SwapBuffers() {
    useBufferA = !useBufferA;
}

ID3D11ShaderResourceView* ParticleSystem::GetCurrentSRV() const {
    return useBufferA ? particleSRVA.get() : particleSRVB.get();
}

ID3D11UnorderedAccessView* ParticleSystem::GetNextUAV() const {
    return useBufferA ? particleUAVB.get() : particleUAVA.get();
}

void ParticleSystem::Reset() {
    InitializeParticles();
    
    // Update buffer A with new data
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(particleBufferA.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
        "Failed to map particle buffer for reset"
    );

    memcpy(mappedResource.pData, particles.data(), sizeof(Particle) * particles.size());
    device->GetContext()->Unmap(particleBufferA.get(), 0);
    
    useBufferA = true;
}

std::vector<Particle> ParticleSystem::ReadBackParticles() {
    auto currentBuffer = useBufferA ? particleBufferA.get() : particleBufferB.get();
    
    // Create staging buffer
    D3D11_BUFFER_DESC stagingDesc = {};
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.ByteWidth = Utils::SafeSizeTToUINT(sizeof(Particle) * particles.size());
    stagingDesc.StructureByteStride = sizeof(Particle);
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    ComPtr<ID3D11Buffer> stagingBuffer;
    THROW_IF_FAILED(
        device->GetDevice()->CreateBuffer(&stagingDesc, nullptr, stagingBuffer.getAddressOf()),
        "Failed to create staging buffer for readback"
    );

    // Copy data
    device->GetContext()->CopyResource(stagingBuffer.get(), currentBuffer);

    // Map and read
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    THROW_IF_FAILED(
        device->GetContext()->Map(stagingBuffer.get(), 0, D3D11_MAP_READ, 0, &mappedResource),
        "Failed to map staging buffer for readback"
    );

    std::vector<Particle> result(particles.size());
    memcpy(result.data(), mappedResource.pData, sizeof(Particle) * particles.size());

    device->GetContext()->Unmap(stagingBuffer.get(), 0);

    return result;
}

#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "Structures.h"
#include <d3d11.h>
#include <vector>
#include <memory>

class ParticleSystem {
private:
    std::shared_ptr<D3DDevice> device;
    SimulationConfig config;
    
    std::vector<Particle> particles;
    
    // Double buffered particle data
    ComPtr<ID3D11Buffer> particleBufferA;
    ComPtr<ID3D11Buffer> particleBufferB;
    ComPtr<ID3D11ShaderResourceView> particleSRVA;
    ComPtr<ID3D11ShaderResourceView> particleSRVB;
    ComPtr<ID3D11UnorderedAccessView> particleUAVA;
    ComPtr<ID3D11UnorderedAccessView> particleUAVB;
    
    // Constant buffer for simulation parameters
    ComPtr<ID3D11Buffer> simulationConstantsBuffer;
    
    bool useBufferA = true; // For double buffering

public:
    explicit ParticleSystem(std::shared_ptr<D3DDevice> device, const SimulationConfig& config);
    ~ParticleSystem() = default;

    bool Initialize();
    void Update(); // Run one simulation step
    void Reset();  // Reset to initial state
    
    // Getters
    ID3D11ShaderResourceView* GetCurrentSRV() const;
    ID3D11UnorderedAccessView* GetNextUAV() const;
    size_t GetParticleCount() const { return particles.size(); }
    const SimulationConfig& GetConfig() const { return config; }
    
    // For debugging
    std::vector<Particle> ReadBackParticles();

private:
    void InitializeParticles();
    bool CreateBuffers();
    bool CreateConstantBuffer();
    void UpdateConstantBuffer();
    void SwapBuffers();
};

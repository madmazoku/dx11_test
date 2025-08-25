#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "ConfigManager.h"
#include "Structures.h"
#include <d3d11.h>
#include <vector>
#include <memory>

// Clean GPU-friendly particle type structure
struct GPUParticleType {
    uint32_t id;
    float mass;
    float radius;
    float charge;
    float padding;
};
// Clean GPU interaction rule structure
struct GPUInteractionRule {
    uint32_t typeA;
    uint32_t typeB;
    uint32_t enabled;
    uint32_t numForces;
    
    float globalStrengthMultiplier;
    float maxInteractionRange;
    float padding[2];
};

// Clean GPU force descriptor
struct GPUForceDescriptor {
    uint32_t type;
    float strength;
    float range;
    float padding;
    float parameters[8];
};

// Clean GPU environmental force
struct GPUEnvironmentalForce {
    uint32_t targetTypeId;
    GPUForceDescriptor force;
    uint32_t enabled;
    float padding[3];
};

class ParticleSystem {
private:
    std::shared_ptr<D3DDevice> device;
    std::shared_ptr<ConfigManager> configManager;
    
    // CPU particle data
    std::vector<Particle> particles;
    std::vector<GPUParticleType> gpuParticleTypes;
    std::vector<GPUInteractionRule> gpuInteractionRules;
    std::vector<GPUForceDescriptor> gpuForceDescriptors;
    std::vector<GPUEnvironmentalForce> gpuEnvironmentalForces;
    
    // GPU buffers for particles (double buffered)
    ComPtr<ID3D11Buffer> particleBufferA;
    ComPtr<ID3D11Buffer> particleBufferB;
    ComPtr<ID3D11ShaderResourceView> particleSRVA;
    ComPtr<ID3D11ShaderResourceView> particleSRVB;
    ComPtr<ID3D11UnorderedAccessView> particleUAVA;
    ComPtr<ID3D11UnorderedAccessView> particleUAVB;
    bool useBufferA = true;
    
    // GPU buffers for particle types and rules
    ComPtr<ID3D11Buffer> particleTypeBuffer;
    ComPtr<ID3D11ShaderResourceView> particleTypeSRV;
    ComPtr<ID3D11Buffer> interactionRuleBuffer;
    ComPtr<ID3D11ShaderResourceView> interactionRuleSRV;
    
    // Simulation constants
    ComPtr<ID3D11Buffer> constantBuffer;
    
    // Helper methods
    void InitializeTypeAndRuleData();
    void InitializeParticles();
    bool CreateParticleBuffers();
    bool CreateTypeBuffers();
    bool CreateRuleBuffers();
    bool CreateConstantBuffer();
    void UpdateConstantBuffer();

public:
    ParticleSystem(std::shared_ptr<D3DDevice> device, 
                   std::shared_ptr<ConfigManager> configManager);
    ~ParticleSystem() = default;

    bool Initialize();
    void Update(std::shared_ptr<ShaderManager> shaderManager);
    
    // Data access
    const std::vector<Particle>& GetParticles() const;
    ID3D11Buffer* GetCurrentParticleBuffer() const;
    ID3D11ShaderResourceView* GetCurrentParticleBufferSRV() const;
    
    // Configuration access
    const std::vector<GPUParticleType>& GetGPUParticleTypes() const { return gpuParticleTypes; }
    const std::vector<GPUInteractionRule>& GetGPUInteractionRules() const { return gpuInteractionRules; }
    
    // Debug/profiling
    void ReadbackParticleData(); // For debugging - expensive operation
    size_t GetParticleCount() const { return particles.size(); }
    size_t GetTypeCount() const { return gpuParticleTypes.size(); }
    size_t GetRuleCount() const { return gpuInteractionRules.size(); }
};

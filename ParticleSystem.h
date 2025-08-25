#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "ConfigManager.h"
#include "Structures.h"
#include <d3d11.h>
#include <vector>
#include <memory>

/**
 * @file ParticleSystem.h
 * @brief Core particle simulation system with GPU acceleration
 * 
 * This file implements the main particle simulation system that manages particle
 * states, force interactions, and GPU compute shader dispatch. The system is
 * designed for high-performance simulation of large numbers of particles with
 * complex multi-type force interactions.
 * 
 * Key features:
 * - GPU-accelerated particle simulation using DirectX compute shaders
 * - Multi-type particle system with configurable interactions
 * - Force-centric architecture supporting various physical forces
 * - Double-buffered GPU memory for efficient parallel processing
 * - Real-time configuration updates and parameter tuning
 * - Memory-efficient GPU data structures
 * - Comprehensive debugging and profiling support
 * 
 * The system bridges the gap between high-level configuration and low-level
 * GPU computation, managing data layout conversion and memory transfers.
 */

/**
 * @struct GPUParticleType
 * @brief GPU-optimized particle type information
 * 
 * Simplified version of ParticleType optimized for GPU memory layout and access.
 * Contains only essential data needed by compute shaders, with proper alignment
 * for efficient GPU memory access patterns.
 */
struct GPUParticleType {
    uint32_t id;            ///< Unique type identifier
    float mass;             ///< Particle mass for force calculations
    float radius;           ///< Particle radius for collision detection
    float charge;           ///< Electric charge for electromagnetic forces
    float padding;          ///< Padding for 16-byte alignment
};

/**
 * @struct GPUInteractionRule
 * @brief GPU-optimized interaction rule for particle types
 * 
 * Compact representation of interaction rules for efficient GPU processing.
 * References force descriptors stored in a separate array to minimize memory
 * usage while maintaining performance.
 */
struct GPUInteractionRule {
    uint32_t typeA;                     ///< First particle type ID
    uint32_t typeB;                     ///< Second particle type ID
    uint32_t enabled;                   ///< Enable flag (1=enabled, 0=disabled)
    uint32_t numForces;                 ///< Number of forces in this interaction
    
    float globalStrengthMultiplier;     ///< Global scaling factor for all forces
    float maxInteractionRange;          ///< Maximum interaction distance
    float padding[2];                   ///< Padding for alignment
};

/**
 * @struct GPUForceDescriptor
 * @brief GPU-optimized force descriptor
 * 
 * Compact representation of force parameters optimized for GPU memory layout.
 * Matches the CPU ForceDescriptor structure but with optimized alignment.
 */
struct GPUForceDescriptor {
    uint32_t type;          ///< Force type (maps to ForceType enum)
    float strength;         ///< Force strength multiplier
    float range;            ///< Maximum interaction range
    float padding;          ///< Alignment padding
    float parameters[8];    ///< Force-specific parameters array
};

/**
 * @struct GPUEnvironmentalForce
 * @brief GPU-optimized environmental force specification
 * 
 * Represents environmental forces that act on particles globally rather than
 * through pairwise interactions. Optimized for efficient GPU processing.
 */
struct GPUEnvironmentalForce {
    uint32_t targetTypeId;      ///< Target particle type (0xFFFFFFFF = all types)
    GPUForceDescriptor force;   ///< Force specification and parameters
    uint32_t enabled;           ///< Enable flag (1=enabled, 0=disabled)
    float padding[3];           ///< Padding for alignment
};

/**
 * @class ParticleSystem
 * @brief Core particle simulation engine with GPU acceleration
 * 
 * This class manages the complete particle simulation pipeline from initialization
 * through real-time updates. It handles the complex task of converting high-level
 * configuration data into GPU-optimized structures and orchestrating compute
 * shader dispatch for parallel particle processing.
 * 
 * Architecture:
 * 1. Configuration Processing: Converts ConfigManager data to GPU structures
 * 2. Memory Management: Maintains double-buffered GPU particle data
 * 3. Compute Dispatch: Coordinates GPU compute shader execution
 * 4. Data Access: Provides interfaces for rendering and analysis
 * 
 * The system uses double buffering to allow the GPU to read from one buffer
 * while writing to another, enabling efficient parallel processing without
 * memory conflicts.
 * 
 * Performance considerations:
 * - Minimizes CPU-GPU memory transfers through persistent GPU buffers
 * - Uses compute shaders for massive parallelization of force calculations
 * - Optimizes memory layout for GPU cache efficiency
 * - Provides asynchronous operation to avoid blocking the render thread
 */
class ParticleSystem {
private:
    // === Dependencies ===
    std::shared_ptr<D3DDevice> device;             ///< DirectX device for GPU operations
    std::shared_ptr<ConfigManager> configManager;   ///< Configuration management system
    
    // === CPU Data Storage ===
    std::vector<Particle> particles;                       ///< CPU copy of particle data for initialization/debugging
    std::vector<GPUParticleType> gpuParticleTypes;         ///< GPU-optimized particle type definitions
    std::vector<GPUInteractionRule> gpuInteractionRules;   ///< GPU-optimized interaction rules
    std::vector<GPUForceDescriptor> gpuForceDescriptors;   ///< GPU-optimized force descriptors
    std::vector<GPUEnvironmentalForce> gpuEnvironmentalForces; ///< GPU-optimized environmental forces
    
    // === Double-Buffered GPU Particle Data ===
    ComPtr<ID3D11Buffer> particleBufferA;          ///< Primary particle buffer
    ComPtr<ID3D11Buffer> particleBufferB;          ///< Secondary particle buffer for double buffering
    ComPtr<ID3D11ShaderResourceView> particleSRVA; ///< Shader resource view for buffer A
    ComPtr<ID3D11ShaderResourceView> particleSRVB; ///< Shader resource view for buffer B
    ComPtr<ID3D11UnorderedAccessView> particleUAVA; ///< Unordered access view for buffer A
    ComPtr<ID3D11UnorderedAccessView> particleUAVB; ///< Unordered access view for buffer B
    bool useBufferA = true;                         ///< Current active buffer selector
    
    // === GPU Configuration Buffers ===
    ComPtr<ID3D11Buffer> particleTypeBuffer;       ///< GPU buffer containing particle type definitions
    ComPtr<ID3D11ShaderResourceView> particleTypeSRV; ///< Shader resource view for particle types
    ComPtr<ID3D11Buffer> interactionRuleBuffer;    ///< GPU buffer containing interaction rules
    ComPtr<ID3D11ShaderResourceView> interactionRuleSRV; ///< Shader resource view for interaction rules
    
    // === Simulation Parameters ===
    ComPtr<ID3D11Buffer> constantBuffer;           ///< GPU constant buffer for simulation parameters
    
    // === Internal Implementation Methods ===
    
    /**
     * @brief Initialize particle type and interaction rule data
     * 
     * Converts configuration data from ConfigManager into GPU-optimized
     * structures and uploads them to GPU buffers.
     */
    void InitializeTypeAndRuleData();
    
    /**
     * @brief Initialize particle positions and properties
     * 
     * Creates initial particle distribution based on configuration
     * settings and uploads initial state to GPU buffers.
     */
    void InitializeParticles();
    
    /**
     * @brief Create double-buffered particle GPU buffers
     * 
     * Allocates and initializes the primary GPU storage for particle data,
     * including both structured buffers and their associated views.
     * 
     * @return true if buffer creation successful, false on error
     */
    bool CreateParticleBuffers();
    
    /**
     * @brief Create GPU buffers for particle type definitions
     * 
     * Creates buffers containing particle type information that compute
     * shaders use for force calculations.
     * 
     * @return true if buffer creation successful, false on error
     */
    bool CreateTypeBuffers();
    
    /**
     * @brief Create GPU buffers for interaction rules
     * 
     * Creates buffers containing force interaction rules between particle
     * types, including force descriptors and environmental forces.
     * 
     * @return true if buffer creation successful, false on error
     */
    bool CreateRuleBuffers();
    
    /**
     * @brief Create constant buffer for simulation parameters
     * 
     * Creates the GPU constant buffer that holds simulation parameters
     * like time step, gravity, boundary conditions, etc.
     * 
     * @return true if buffer creation successful, false on error
     */
    bool CreateConstantBuffer();
    
    /**
     * @brief Update simulation constant buffer with current parameters
     * 
     * Updates the GPU constant buffer with current simulation parameters
     * from the configuration, allowing real-time parameter changes.
     */
    void UpdateConstantBuffer();

public:
    // === Construction and Lifecycle ===
    
    /**
     * @brief Construct particle system with required dependencies
     * 
     * @param device DirectX device for GPU resource creation
     * @param configManager Configuration manager for simulation parameters
     */
    ParticleSystem(std::shared_ptr<D3DDevice> device, 
                   std::shared_ptr<ConfigManager> configManager);
    
    /**
     * @brief Default destructor
     * 
     * GPU resources are automatically cleaned up by ComPtr destructors
     */
    ~ParticleSystem() = default;

    // === Core Operations ===
    
    /**
     * @brief Initialize the particle simulation system
     * 
     * Performs complete initialization including:
     * - Converting configuration to GPU-optimized structures
     * - Creating all necessary GPU buffers and resources
     * - Initializing particle positions and properties
     * - Setting up force interaction rules
     * 
     * Must be called before any simulation updates.
     * 
     * @return true if initialization successful, false on error
     */
    bool Initialize();
    
    /**
     * @brief Update particle simulation for one time step
     * 
     * Dispatches GPU compute shaders to update all particle positions,
     * velocities, and forces based on current configuration parameters.
     * Uses double buffering to avoid memory conflicts during parallel processing.
     * 
     * @param shaderManager Shader manager for compute shader access
     */
    void Update(std::shared_ptr<ShaderManager> shaderManager);
    
    // === Data Access Methods ===
    
    /**
     * @brief Get CPU copy of particle data
     * 
     * Returns reference to CPU-side particle data. Note that this may not
     * reflect the current GPU state unless ReadbackParticleData() has been
     * called recently.
     * 
     * @return Reference to CPU particle data vector
     */
    const std::vector<Particle>& GetParticles() const;
    
    /**
     * @brief Get currently active particle GPU buffer
     * 
     * Returns the GPU buffer containing the most recent particle data.
     * Used by rendering system and other GPU-based operations.
     * 
     * @return Pointer to current particle buffer
     */
    ID3D11Buffer* GetCurrentParticleBuffer() const;
    
    /**
     * @brief Get shader resource view for current particle buffer
     * 
     * Returns the SRV for the currently active particle buffer, suitable
     * for use as input to compute or graphics shaders.
     * 
     * @return Pointer to current particle buffer SRV
     */
    ID3D11ShaderResourceView* GetCurrentParticleBufferSRV() const;
    
    // === Configuration Access ===
    
    /**
     * @brief Get GPU-optimized particle type definitions
     * @return Reference to GPU particle type vector
     */
    const std::vector<GPUParticleType>& GetGPUParticleTypes() const { return gpuParticleTypes; }
    
    /**
     * @brief Get GPU-optimized interaction rules
     * @return Reference to GPU interaction rule vector
     */
    const std::vector<GPUInteractionRule>& GetGPUInteractionRules() const { return gpuInteractionRules; }
    
    // === Debug and Profiling Methods ===
    
    /**
     * @brief Read particle data back from GPU to CPU
     * 
     * Performs expensive synchronous readback of particle data from GPU
     * memory to CPU memory. Should only be used for debugging or analysis
     * as it causes GPU pipeline stalls.
     * 
     * WARNING: This is a costly operation that blocks the GPU pipeline.
     */
    void ReadbackParticleData();
    
    /**
     * @brief Get total number of particles in simulation
     * @return Number of particles being simulated
     */
    size_t GetParticleCount() const { return particles.size(); }
    
    /**
     * @brief Get number of particle types defined
     * @return Number of different particle types
     */
    size_t GetTypeCount() const { return gpuParticleTypes.size(); }
    
    /**
     * @brief Get number of interaction rules defined
     * @return Number of force interaction rules
     */
    size_t GetRuleCount() const { return gpuInteractionRules.size(); }
};

#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>

using namespace DirectX;

/**
 * @file Structures.h
 * @brief Core data structures for the multi-type particle simulation system
 * 
 * This file defines all the fundamental data structures used throughout the particle
 * simulation system. The design follows a force-centric architecture where particles
 * interact through various physical forces rather than hardcoded behaviors.
 * 
 * Key architectural principles:
 * - Force-centric design: All particle interactions are modeled as physical forces
 * - Multi-type system: Different particle types with unique properties and interactions
 * - Modular force system: Forces can be easily added, removed, and configured
 * - GPU-friendly layout: Structures designed for efficient GPU computation
 * - Extensible parameters: Force descriptors use parameter arrays for flexibility
 * 
 * The system supports various force types including springs, Lennard-Jones potentials,
 * gravitational forces, electromagnetic interactions, viscous drag, and field forces.
 */

/**
 * @enum ForceType
 * @brief Enumeration of available force interaction types
 * 
 * This enumeration defines all the physical force models available in the simulation.
 * Each force type implements a specific physical law or interaction model, allowing
 * for realistic and scientifically-based particle behaviors.
 * 
 * The force types are designed to be:
 * - Physically accurate: Based on real physical laws and equations
 * - Computationally efficient: Optimized for GPU parallel computation  
 * - Modular: Can be combined in different ways for complex behaviors
 * - Parameterizable: Each force type accepts custom parameters for tuning
 */
enum class ForceType : uint32_t {
    None = 0,             ///< No force interaction (disabled)
    Spring = 1,           ///< Hooke's law spring force: F = -k(r - r0), provides elastic connections
    LennardJones = 2,     ///< Lennard-Jones potential: attractive at distance, repulsive when close
    Gravitational = 3,    ///< Newton's gravitational law: F = G*m1*m2/r², always attractive
    Electromagnetic = 4,  ///< Coulomb's law: F = k*q1*q2/r², attractive or repulsive based on charge
    Viscous = 5,          ///< Viscous drag force: F = -β*v, opposes motion through medium
    ConstantField = 6,    ///< Constant force field: F = constant vector (like uniform gravity)
    RadialField = 7,      ///< Radial force from center: F ∝ 1/r² from fixed point
    VortexField = 8       ///< Rotational vortex force: creates swirling motion around axis
};

/**
 * @struct MaterialProperties
 * @brief Material properties for physically-based Phong shading model
 * 
 * Defines the visual appearance of particles using a hybrid of traditional Phong
 * shading and physically-based rendering principles. These properties control
 * how particles reflect and emit light, creating realistic visual materials.
 * 
 * The material model supports:
 * - Diffuse reflection: Lambert's law for matte surfaces
 * - Specular reflection: Phong model with configurable shininess
 * - Metallic workflow: PBR-style metallic/roughness parameters
 * - Emissive materials: Self-illuminating surfaces
 * - Energy conservation: Realistic light interaction
 */
struct MaterialProperties {
    XMFLOAT3 diffuseColor = { 1.0f, 1.0f, 1.0f };    ///< Base albedo color (RGB)
    XMFLOAT3 specularColor = { 1.0f, 1.0f, 1.0f };   ///< Specular highlight tint
    float shininess = 32.0f;                          ///< Specular exponent (higher = sharper highlights)
    float metallic = 0.0f;                           ///< Metallic factor: 0.0=dielectric, 1.0=conductor
    float roughness = 0.5f;                          ///< Surface microsurface roughness (0=mirror, 1=rough)
    float reflectance = 0.04f;                       ///< Base reflectance for dielectrics (F0)
    float emissive = 0.0f;                          ///< Emissive intensity multiplier
    XMFLOAT3 emissiveColor = { 0.0f, 0.0f, 0.0f };   ///< Self-illumination color
};

/**
 * @struct ForceDescriptor
 * @brief Complete specification for a single force interaction
 * 
 * This structure encapsulates all the parameters needed to define a force interaction
 * between particles. It uses a flexible parameter array to accommodate the different
 * parameter requirements of various force types, making the system highly extensible.
 * 
 * The design supports:
 * - Type-safe force specification through ForceType enum
 * - Flexible parameterization via parameter array
 * - Range-limited interactions for performance
 * - Strength scaling for easy force balancing
 * 
 * Parameter array usage is documented below for each force type to ensure
 * consistent interpretation across the CPU and GPU code paths.
 */
struct ForceDescriptor {
    ForceType type = ForceType::None;    ///< Type of force interaction to apply
    float strength = 1.0f;               ///< Global strength multiplier for the force
    float range = 2.0f;                  ///< Maximum interaction distance (culling optimization)
    
    /**
     * @brief Type-specific parameter array
     * 
     * Flexible parameter storage for different force types. The interpretation
     * of each array element depends on the ForceType:
     * 
     * Spring Force:
     *   [0] = stiffness coefficient (k in F = -k*x)
     *   [1] = damping coefficient (b in F = -b*v)  
     *   [2] = rest length (natural spring length)
     * 
     * Lennard-Jones:
     *   [0] = epsilon (depth of potential well)
     *   [1] = sigma (distance at which potential is zero)
     * 
     * Gravitational:
     *   [0] = gravitational constant (G)
     * 
     * Electromagnetic:
     *   [0] = Coulomb constant (k_e)
     * 
     * Viscous Drag:
     *   [0] = linear viscosity coefficient
     *   [1] = quadratic drag coefficient
     * 
     * Constant Field:
     *   [0-2] = field vector components (x, y, z)
     *   [3] = mass scaling factor
     * 
     * Radial Field:
     *   [0-2] = center position (x, y, z)
     *   [3] = field strength
     *   [4] = repulsive flag (1.0 = repulsive, 0.0 = attractive)
     * 
     * Vortex Field:
     *   [0-2] = center position (x, y, z)
     *   [3-5] = rotation axis (normalized vector)
     *   [6] = vortex strength
     */
    float parameters[8] = {0};           ///< Force-specific parameters array
};

/**
 * @struct ParticleType
 * @brief Definition of a particle type with physical and visual properties
 * 
 * Defines a class of particles with shared properties. The multi-type particle system
 * allows different particle types to have distinct physical characteristics and
 * visual appearances, enabling complex heterogeneous simulations.
 * 
 * Each particle type specifies:
 * - Core physical properties (mass, size, charge)
 * - Visual rendering properties (material, LOD settings)
 * - Default values that can be overridden per-particle
 * 
 * The separation of physical and visual properties ensures that force calculations
 * remain independent of rendering concerns.
 */
struct ParticleType {
    uint32_t id = 0;                    ///< Unique identifier for this particle type
    std::string name = "Default";       ///< Human-readable name for debugging/UI
    
    // === Essential Physical Properties ===
    // These properties directly affect force calculations and simulation behavior
    
    float mass = 1.0f;                  ///< Particle mass [kg] - affects inertia and gravitational forces
    float radius = 0.1f;                ///< Particle radius [m] - for collision detection and contact forces
    float charge = 0.0f;                ///< Electric charge [C] - for electromagnetic forces (0 = neutral)
    
    // === Visual Properties ===  
    // These properties only affect rendering and have no impact on physics
    
    MaterialProperties material;         ///< Surface material for shading calculations
    uint32_t lodLevels = 3;             ///< Maximum icosphere subdivision levels for rendering
};

/**
 * @struct InteractionRule
 * @brief Defines force interactions between two specific particle types
 * 
 * This structure specifies how two particle types interact with each other through
 * one or more simultaneous forces. It enables complex multi-force interactions
 * where particles can experience multiple physical phenomena simultaneously.
 * 
 * Key features:
 * - Multi-force interactions: Multiple forces can act between the same particle types
 * - Selective interactions: Specify exactly which types interact
 * - Global modifiers: Apply scaling factors to all forces in the interaction
 * - Range optimization: Set maximum interaction range for performance
 * 
 * For example, two particle types might interact through both Lennard-Jones
 * attraction/repulsion AND electromagnetic forces simultaneously.
 */
struct InteractionRule {
    uint32_t typeA = 0;                         ///< ID of first particle type in interaction
    uint32_t typeB = 0;                         ///< ID of second particle type in interaction
    bool enabled = true;                        ///< Enable/disable this entire interaction rule
    
    std::vector<ForceDescriptor> forces;        ///< List of forces acting between these types
    
    // === Global Modifiers ===
    // These modifiers affect ALL forces in this interaction rule
    
    float globalStrengthMultiplier = 1.0f;      ///< Scale factor applied to all forces
    float maxInteractionRange = 10.0f;          ///< Maximum distance for any force in this rule
};

/**
 * @struct EnvironmentalForce
 * @brief Environmental force that affects specific particle types globally
 * 
 * Environmental forces act on particles regardless of other particles' positions,
 * representing external field effects like gravity, wind, electric fields, etc.
 * These forces can target all particles or specific particle types.
 * 
 * Unlike interaction forces (which act between particle pairs), environmental
 * forces represent external influences that affect particles individually.
 * Examples include gravitational fields, electromagnetic fields, fluid currents.
 */
struct EnvironmentalForce {
    uint32_t targetTypeId = 0xFFFFFFFF;     ///< Target particle type (0xFFFFFFFF = all types)
    ForceDescriptor force;                  ///< Force specification and parameters
    bool enabled = true;                    ///< Enable/disable this environmental force
};

/**
 * @struct SimulationConfig
 * @brief Complete configuration for the particle simulation system
 * 
 * This is the master configuration structure that defines all aspects of the
 * particle simulation, from basic parameters to complex multi-type interactions.
 * It serves as the single source of truth for simulation behavior.
 * 
 * The configuration is designed to be:
 * - Serializable: Can be saved/loaded from JSON files
 * - Comprehensive: Covers all simulation aspects
 * - Modular: Components can be configured independently
 * - Extensible: Easy to add new configuration parameters
 * 
 * Major configuration categories:
 * - Core simulation parameters (time step, particle count)
 * - Multi-type particle system definition
 * - Force interaction specifications
 * - Performance and optimization settings
 * - Boundary conditions and constraints
 * - Particle spawn and distribution settings
 */
struct SimulationConfig {
    // === Core Simulation Parameters ===
    
    size_t particleCount = 256;             ///< Total number of particles in simulation
    float timeStep = 0.016f;                ///< Integration time step [s] (0.016 ≈ 60fps)
    int maxFrames = 0;                      ///< Maximum frames to simulate (0 = infinite)
    bool enableDebugOutput = true;          ///< Enable debug logging and statistics
    int debugOutputInterval = 60;          ///< Debug output frequency [frames]
    
    // === Multi-Type Particle System ===
    
    std::vector<ParticleType> particleTypes;        ///< Definitions of all particle types
    std::vector<InteractionRule> interactionRules; ///< Force interactions between types
    std::vector<EnvironmentalForce> environmentalForces; ///< Global environmental forces
    
    // === Performance and Optimization Settings ===
    
    uint32_t maxInteractionsPerParticle = 50;       ///< Limit interactions for performance
    float spatialGridSize = 2.0f;                   ///< Grid cell size for spatial optimization
    bool enableSpatialOptimization = true;          ///< Use spatial hashing for performance
    bool enableMultiThreading = true;               ///< Enable CPU multi-threading
    
    // === Boundary Conditions ===
    
    XMFLOAT3 boundaryMin = { -10.0f, -10.0f, -10.0f }; ///< Minimum boundary coordinates
    XMFLOAT3 boundaryMax = { 10.0f, 10.0f, 10.0f };    ///< Maximum boundary coordinates  
    float boundaryRestitution = 0.8f;                   ///< Elasticity of boundary collisions
    
    // === Particle Distribution Settings ===
    
    /**
     * @struct TypeDistribution  
     * @brief Configuration for spawning particles of a specific type
     * 
     * Defines how particles of a specific type should be distributed in the
     * simulation space at initialization. Supports various distribution patterns
     * and per-particle property variations for realistic scenarios.
     */
    struct TypeDistribution {
        uint32_t typeId = 0;                        ///< ID of particle type to spawn
        float percentage = 1.0f;                    ///< Fraction of total particles [0.0-1.0]
        XMFLOAT3 spawnCenter = { 0.0f, 0.0f, 0.0f }; ///< Center point for spawning
        float spawnRadius = 1.0f;                   ///< Radius of spawn distribution
        XMFLOAT3 initialVelocity = { 0.0f, 0.0f, 0.0f }; ///< Initial velocity for all particles
        
        // === Property Variation Settings ===
        // Add randomness to make simulations more realistic
        
        float massVariation = 0.0f;                 ///< ±% random mass variation per particle
        float radiusVariation = 0.0f;               ///< ±% random radius variation per particle
    };
    std::vector<TypeDistribution> typeDistribution; ///< Distribution rules for all types
};

/**
 * @struct Particle
 * @brief Runtime state of a single particle in the simulation
 * 
 * This structure represents the complete state of one particle during simulation.
 * It contains both the current state (position, velocity) and derived properties
 * that are computed each frame for force calculations and rendering.
 * 
 * The structure is designed for efficient GPU processing:
 * - Aligned memory layout for optimal GPU memory access
 * - Minimal data size while retaining necessary information
 * - Separation of frequently updated vs. static data
 * 
 * State management:
 * - position/velocity: Current kinematic state
 * - oldPosition: For Verlet integration schemes
 * - acceleration: Accumulated forces from current frame
 * - typeId: Links to ParticleType for properties lookup
 */
struct Particle {
    // === Kinematic State ===
    // Core state variables for physics integration
    
    XMFLOAT3 position;                  ///< Current world position [m]
    XMFLOAT3 oldPosition;               ///< Previous frame position (for Verlet integration)
    XMFLOAT3 velocity;                  ///< Current velocity vector [m/s]
    XMFLOAT3 acceleration;              ///< Current acceleration (accumulated forces) [m/s²]
    
    // === Type and Properties ===
    // Particle classification and current property values
    
    uint32_t typeId;                    ///< ID linking to ParticleType definition
    float mass;                         ///< Current mass [kg] (may differ from type default)
    float radius;                       ///< Current radius [m] (may differ from type default)
    float charge;                       ///< Current electric charge [C]
    
    // === State Variables ===
    // Additional simulation state for advanced behaviors
    
    float temperature;                  ///< Current temperature [K] (for thermal effects)
    float age;                          ///< Time since particle creation [s]
    uint32_t collisionCount;            ///< Number of collisions this frame
    uint32_t flags;                     ///< Bit flags (active, collision enabled, etc.)
};

/**
 * @struct CullingConfig
 * @brief Configuration for rendering optimization through culling techniques
 * 
 * This structure controls various culling and Level-of-Detail (LOD) techniques
 * used to optimize rendering performance. Culling eliminates unnecessary
 * rendering work by skipping particles that won't contribute to the final image.
 * 
 * Supported optimization techniques:
 * - Frustum culling: Skip particles outside camera view
 * - Distance culling: Skip particles beyond maximum render distance
 * - LOD system: Reduce detail for distant particles
 * - Backface culling: Skip back-facing geometry
 * - Size-based culling: Skip particles too small to see
 */
struct CullingConfig {
    bool enableFrustumCulling = true;       ///< Enable camera frustum culling
    float maxRenderDistance = 100.0f;       ///< Maximum render distance [world units]
    
    // === Level-of-Detail (LOD) Distances ===
    // Define distance thresholds for different detail levels
    
    float lodDistance1 = 10.0f;             ///< Transition to medium detail [world units]
    float lodDistance2 = 25.0f;             ///< Transition to low detail [world units] 
    float lodDistance3 = 50.0f;             ///< Transition to minimal detail [world units]
    
    // === Culling Enable Flags ===
    
    bool enableLOD = true;                  ///< Enable distance-based LOD system
    bool enableBackfaceCulling = true;      ///< Enable backface culling for geometry
    bool enableDistanceCulling = true;      ///< Enable maximum distance culling
    
    // === Size and Scale Parameters ===
    
    float globalScale = 1.0f;               ///< Global scale factor for all particles
    float minParticleRadius = 0.05f;        ///< Minimum particle radius for culling
    float maxParticleRadius = 0.2f;         ///< Maximum particle radius for culling
};

/**
 * @struct IcosphereConfig  
 * @brief Configuration for icosphere-based particle rendering
 * 
 * Controls the generation and rendering of icosphere geometry used to represent
 * particles as 3D spheres. Icospheres provide more uniform triangle distribution
 * compared to UV spheres, making them ideal for particle representation.
 * 
 * Features:
 * - Adaptive LOD: Automatically reduce detail based on distance
 * - Configurable subdivision: Control triangle density
 * - Shading options: Flat vs smooth normal interpolation
 * - Debug visualization: Wireframe mode for debugging
 */
struct IcosphereConfig {
    uint32_t maxSubdivisions = 3;           ///< Maximum icosphere subdivision levels (0-4)
    bool enableAdaptiveLOD = true;          ///< Use distance-based subdivision levels
    
    // === LOD Distance Thresholds ===
    // Define when to switch between subdivision levels
    
    float lodDistance0 = 10.0f;             ///< High detail distance (max subdivisions)
    float lodDistance1 = 25.0f;             ///< Medium detail distance (mid subdivisions)
    float lodDistance2 = 50.0f;             ///< Low detail distance (min subdivisions)
    
    // === Rendering Options ===
    
    bool enableFlatShading = false;         ///< Use flat normals (false = smooth normals)
    bool enableWireframe = false;           ///< Render wireframe for debugging
};

/**
 * @struct RenderConfig
 * @brief Complete rendering system configuration
 * 
 * This structure contains all settings related to visual rendering of the particle
 * simulation. It encompasses window settings, camera controls, lighting parameters,
 * and all visual optimization settings.
 * 
 * The configuration is organized into logical sections:
 * - Display settings: Window size, VSync, background
 * - Camera system: Interactive controls and view parameters  
 * - Lighting model: Directional light and ambient settings
 * - Optimization: Culling and LOD configurations
 * 
 * All settings are designed to be runtime-configurable for easy experimentation
 * and optimization.
 */
struct RenderConfig {
    // === Display Settings ===
    
    uint32_t windowWidth = 1440;                    ///< Render window width [pixels]
    uint32_t windowHeight = 900;                    ///< Render window height [pixels]
    bool enableVSync = true;                        ///< Enable vertical synchronization
    float clearColor[4] = { 0.05f, 0.05f, 0.15f, 1.0f }; ///< Background clear color (RGBA)
    
    // === Basic Camera Settings ===
    // Default orbit camera parameters when interactive camera is disabled
    
    float cameraRadius = 15.0f;                     ///< Distance from origin for orbit camera
    float cameraHeight = 3.0f;                      ///< Height offset for orbit camera
    float cameraRotationSpeed = 0.8f;               ///< Rotation speed for orbit camera
    
    // === Camera Projection ===
    
    float cameraFov = 50.0f;                        ///< Field of view [degrees]
    float cameraNearPlane = 0.1f;                   ///< Near clipping plane [world units]
    float cameraFarPlane = 200.0f;                  ///< Far clipping plane [world units]
    
    // === Interactive Camera Controls ===
    
    bool enableInteractiveCamera = true;            ///< Enable mouse/keyboard camera control
    float cameraZoomMin = 0.05f;                    ///< Minimum zoom factor
    float cameraZoomMax = 1.0f;                     ///< Maximum zoom factor  
    float cameraZoomStep = 0.03f;                   ///< Zoom increment per scroll step
    float cameraRotationSensitivity = 0.008f;      ///< Mouse sensitivity for rotation
    bool enableAutoCentering = true;                ///< Auto-center camera on particle system
    
    // === Lighting Model ===
    
    float lightDirection[3] = { -0.5f, -0.8f, -0.6f }; ///< Directional light direction (normalized)
    float lightColor[3] = { 1.0f, 0.95f, 0.9f };       ///< Light color (RGB)
    float lightIntensity = 1.2f;                        ///< Light intensity multiplier
    float ambientIntensity = 0.3f;                      ///< Ambient light intensity
    float specularPower = 64.0f;                        ///< Specular highlight sharpness
    
    // === Optimization Configurations ===
    
    CullingConfig culling;                          ///< Frustum culling and LOD settings
    IcosphereConfig icosphere;                      ///< Icosphere rendering settings
};

/**
 * @struct SimulationConstants
 * @brief GPU constant buffer for compute shader simulation parameters
 * 
 * This structure is passed to GPU compute shaders to control particle simulation
 * behavior. It must maintain 16-byte alignment for DirectX constant buffer
 * requirements and contains all parameters needed for force calculations.
 * 
 * The structure includes:
 * - Integration parameters (time step, damping)
 * - Environmental forces (gravity, wind, temperature)
 * - Boundary conditions and collision parameters
 * - Multi-type system configuration
 * - Spatial optimization settings
 * 
 * All members must be aligned to 16-byte boundaries for GPU compatibility.
 */
struct alignas(16) SimulationConstants {
    // === Integration Parameters ===
    
    float deltaTime;                    ///< Time step for numerical integration [s]
    float globalDamping;                ///< Global velocity damping factor [0-1]
    float padding1[2];                  ///< Padding for 16-byte alignment
    
    // === Environmental Forces ===
    
    XMFLOAT3 gravity;                   ///< Global gravity vector [m/s²]
    float boundaryRestitution;          ///< Collision elasticity with boundaries [0-1]
    
    XMFLOAT3 boundaryMin;               ///< Minimum boundary coordinates [world units]
    float airDensity;                   ///< Air density for drag calculations [kg/m³]
    
    XMFLOAT3 boundaryMax;               ///< Maximum boundary coordinates [world units]
    float fluidViscosity;               ///< Fluid viscosity for drag forces [Pa·s]
    
    XMFLOAT3 windVelocity;              ///< Environmental wind velocity [m/s]
    float ambientTemperature;           ///< Ambient temperature [K]
    
    // === Multi-Type System Parameters ===
    
    uint32_t numParticleTypes;          ///< Number of particle types in simulation
    uint32_t numInteractionRules;       ///< Number of interaction rules defined
    uint32_t maxInteractionsPerParticle; ///< Maximum interactions per particle (optimization)
    float spatialGridSize;              ///< Grid cell size for spatial hashing [world units]
    
    // === Advanced Parameters ===
    
    float thermalDiffusion;             ///< Thermal diffusion coefficient [m²/s]
    float padding2[3];                  ///< Padding for 16-byte alignment
};

/**
 * @struct TransformBuffer
 * @brief GPU constant buffer for vertex shader transformation matrices
 * 
 * Contains transformation matrices and camera information needed by vertex
 * shaders to transform particle geometry from world space to screen space.
 * Used for both particle rendering and icosphere generation.
 */
struct alignas(16) TransformBuffer {
    XMMATRIX viewProjectionMatrix;      ///< Combined view * projection matrix
    XMMATRIX worldMatrix;               ///< World transformation matrix
    XMFLOAT3 cameraPos;                 ///< Camera position in world space
    float globalScale;                  ///< Global scale factor for all geometry
};

/**
 * @struct LightBuffer  
 * @brief GPU constant buffer for lighting calculations in pixel shaders
 * 
 * Contains all lighting parameters needed for Phong shading model including
 * directional light properties, ambient lighting, and camera position for
 * specular calculations.
 */
struct alignas(16) LightBuffer {
    XMFLOAT3 lightDirection;            ///< Normalized directional light direction
    float lightIntensity;               ///< Light intensity multiplier
    XMFLOAT3 lightColor;                ///< Light color (RGB)
    float ambientIntensity;             ///< Ambient light intensity
    XMFLOAT3 cameraPos;                 ///< Camera position for specular calculations
    float specularPower;                ///< Specular highlight sharpness exponent
};

/**
 * @struct MaterialBuffer
 * @brief GPU constant buffer for material properties in pixel shaders
 * 
 * Contains material properties for physically-based Phong shading including
 * diffuse/specular colors, metallic workflow parameters, and emissive properties.
 * Used to define the visual appearance of particle materials.
 */
struct alignas(16) MaterialBuffer {
    XMFLOAT3 diffuseColor;              ///< Base material color (albedo)
    float metallic;                     ///< Metallic factor [0-1]
    XMFLOAT3 specularColor;             ///< Specular highlight color
    float roughness;                    ///< Surface roughness [0-1]
    float shininess;                    ///< Specular exponent
    float reflectance;                  ///< Base reflectance for dielectrics  
    float emissive;                     ///< Emissive intensity
    float padding1;                     ///< Padding for alignment
    XMFLOAT3 emissiveColor;             ///< Emissive color
    float padding2;                     ///< Padding for alignment
};

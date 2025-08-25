#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>

using namespace DirectX;

// Force interaction types - clean enumeration
enum class ForceType : uint32_t {
    None = 0,
    Spring = 1,           // Hooke's law: F = -k(r - r0)
    LennardJones = 2,     // Lennard-Jones potential: attractive/repulsive
    Gravitational = 3,    // Newton's law: F = G*m1*m2/r²
    Electromagnetic = 4,  // Coulomb's law: F = k*q1*q2/r²
    Viscous = 5,          // Stokes drag: F = -β*v
    ConstantField = 6,    // Constant force field (environmental)
    RadialField = 7,      // Radial force from center point
    VortexField = 8       // Rotational force field
};

// Material properties for Phong shading - clean visual-only properties
struct MaterialProperties {
    XMFLOAT3 diffuseColor = { 1.0f, 1.0f, 1.0f };    // Base color
    XMFLOAT3 specularColor = { 1.0f, 1.0f, 1.0f };   // Specular highlight color
    float shininess = 32.0f;                          // Specular exponent
    float metallic = 0.0f;                           // Metallic factor (0.0 = dielectric, 1.0 = metallic)
    float roughness = 0.5f;                          // Surface roughness
    float reflectance = 0.04f;                       // Base reflectance for dielectrics
    float emissive = 0.0f;                          // Emissive strength
    XMFLOAT3 emissiveColor = { 0.0f, 0.0f, 0.0f };   // Emissive color
};

// Single force descriptor - clean parameter structure
struct ForceDescriptor {
    ForceType type = ForceType::None;
    float strength = 1.0f;           // Force multiplier
    float range = 2.0f;              // Maximum interaction distance
    
    // Type-specific parameters (flexible array for different force types)
    float parameters[8] = {0};       // Enough for any force type parameters
    
    // Usage guide for parameters array:
    // Spring: [0]=stiffness, [1]=dampingCoeff, [2]=restLength
    // LennardJones: [0]=epsilon, [1]=sigma
    // Gravitational: [0]=gravitationalConstant
    // Electromagnetic: [0]=coulombConstant
    // Viscous: [0]=viscosityCoeff, [1]=dragCoeff
    // ConstantField: [0-2]=fieldVector(x,y,z), [3]=massMultiplier
    // RadialField: [0-2]=center(x,y,z), [3]=fieldStrength, [4]=isRepulsive(1/0)
    // VortexField: [0-2]=center(x,y,z), [3-5]=axis(x,y,z), [6]=strength
};

// Clean particle type information - only essentials
struct ParticleType {
    uint32_t id = 0;
    std::string name = "Default";
    
    // Essential physical properties - only what forces need
    float mass = 1.0f;              // Mass for gravitational/inertial forces
    float radius = 0.1f;            // Radius for collision/contact forces
    
    // Optional properties for specific force types (0 = not used)
    float charge = 0.0f;            // Electric charge for electromagnetic forces
    
    // Visual properties only
    MaterialProperties material;
    uint32_t lodLevels = 3;         // Icosphere subdivision levels
};

// Force interaction between two particle types - can have multiple forces
struct InteractionRule {
    uint32_t typeA = 0;
    uint32_t typeB = 0;
    bool enabled = true;
    
    // Multiple forces acting simultaneously between these types
    std::vector<ForceDescriptor> forces;
    
    // Global modifiers for all forces in this interaction
    float globalStrengthMultiplier = 1.0f;
    float maxInteractionRange = 10.0f;
};

// Environmental force acting on specific particle type(s)
struct EnvironmentalForce {
    uint32_t targetTypeId = 0xFFFFFFFF;  // Which type (0xFFFFFFFF = all types)
    ForceDescriptor force;
    bool enabled = true;
};

// Simulation configuration - now much cleaner
struct SimulationConfig {
    size_t particleCount = 256;
    float timeStep = 0.016f;  // 60fps
    int maxFrames = 0;        // 0 = infinite
    bool enableDebugOutput = true;
    int debugOutputInterval = 60;
    
    // Multi-type system
    std::vector<ParticleType> particleTypes;
    std::vector<InteractionRule> interactionRules;
    std::vector<EnvironmentalForce> environmentalForces;
    
    // Performance settings
    uint32_t maxInteractionsPerParticle = 50;
    float spatialGridSize = 2.0f;
    bool enableSpatialOptimization = true;
    bool enableMultiThreading = true;
    
    // Boundary constraints
    XMFLOAT3 boundaryMin = { -10.0f, -10.0f, -10.0f };
    XMFLOAT3 boundaryMax = { 10.0f, 10.0f, 10.0f };
    float boundaryRestitution = 0.8f;
    
    // Particle distribution
    struct TypeDistribution {
        uint32_t typeId = 0;
        float percentage = 1.0f;        // 0.0-1.0
        XMFLOAT3 spawnCenter = { 0.0f, 0.0f, 0.0f };
        float spawnRadius = 1.0f;
        XMFLOAT3 initialVelocity = { 0.0f, 0.0f, 0.0f };
        
        // Optional per-particle property overrides
        float massVariation = 0.0f;     // ±% mass variation
        float radiusVariation = 0.0f;   // ±% radius variation
    };
    std::vector<TypeDistribution> typeDistribution;
};

// Unified particle structure for multi-type system
struct Particle {
    XMFLOAT3 position;
    XMFLOAT3 oldPosition;
    XMFLOAT3 velocity;
    XMFLOAT3 acceleration;
    
    // Type and properties
    uint32_t typeId;               // Particle type identifier
    float mass;                    // Current mass (can differ from type default)
    float radius;                  // Current radius (can differ from type default)
    float charge;                  // Current electric charge
    
    // State variables
    float temperature;             // Current temperature
    float age;                     // Particle age in seconds
    uint32_t collisionCount;       // Collisions this frame
    uint32_t flags;               // Various flags (active, collision enabled, etc.)
};

// Culling configuration
struct CullingConfig {
    bool enableFrustumCulling = true;
    float maxRenderDistance = 100.0f;
    float lodDistance1 = 10.0f;   // High detail
    float lodDistance2 = 25.0f;   // Medium detail
    float lodDistance3 = 50.0f;   // Low detail
    bool enableLOD = true;
    bool enableBackfaceCulling = true;
    bool enableDistanceCulling = true;
    float globalScale = 1.0f;
    float minParticleRadius = 0.05f;
    float maxParticleRadius = 0.2f;
};

// Icosphere rendering configuration
struct IcosphereConfig {
    uint32_t maxSubdivisions = 3;     // Maximum LOD subdivisions
    bool enableAdaptiveLOD = true;    // Distance-based LOD
    float lodDistance0 = 10.0f;       // High detail distance
    float lodDistance1 = 25.0f;       // Medium detail distance  
    float lodDistance2 = 50.0f;       // Low detail distance
    bool enableFlatShading = false;   // Flat vs smooth normals
    bool enableWireframe = false;     // Debug wireframe rendering
};

struct RenderConfig {
    uint32_t windowWidth = 1440;
    uint32_t windowHeight = 900;
    bool enableVSync = true;
    float clearColor[4] = { 0.05f, 0.05f, 0.15f, 1.0f };
    
    // Camera settings
    float cameraRadius = 15.0f;
    float cameraHeight = 3.0f;
    float cameraRotationSpeed = 0.8f;
    float cameraFov = 50.0f;
    float cameraNearPlane = 0.1f;
    float cameraFarPlane = 200.0f;
    
    // Interactive camera settings
    bool enableInteractiveCamera = true;
    float cameraZoomMin = 0.05f;
    float cameraZoomMax = 1.0f;
    float cameraZoomStep = 0.03f;
    float cameraRotationSensitivity = 0.008f;
    bool enableAutoCentering = true;
    
    // Lighting settings
    float lightDirection[3] = { -0.5f, -0.8f, -0.6f };
    float lightColor[3] = { 1.0f, 0.95f, 0.9f };
    float lightIntensity = 1.2f;
    float ambientIntensity = 0.3f;
    float specularPower = 64.0f;
    
    // Culling settings
    CullingConfig culling;
    
    // Icosphere rendering
    IcosphereConfig icosphere;
};

// Compute shader constant buffer for advanced multi-type simulation
struct alignas(16) SimulationConstants {
    float deltaTime;
    float globalDamping;
    float padding1[2];
    
    XMFLOAT3 gravity;
    float boundaryRestitution;
    
    XMFLOAT3 boundaryMin;
    float airDensity;
    
    XMFLOAT3 boundaryMax;
    float fluidViscosity;
    
    XMFLOAT3 windVelocity;
    float ambientTemperature;
    
    uint32_t numParticleTypes;
    uint32_t numInteractionRules;
    uint32_t maxInteractionsPerParticle;
    float spatialGridSize;
    
    float thermalDiffusion;
    float padding2[3];
};

// Constant buffer structures for rendering
struct alignas(16) TransformBuffer {
    XMMATRIX viewProjectionMatrix;
    XMMATRIX worldMatrix;
    XMFLOAT3 cameraPos;
    float globalScale;
};

struct alignas(16) LightBuffer {
    XMFLOAT3 lightDirection;
    float lightIntensity;
    XMFLOAT3 lightColor;
    float ambientIntensity;
    XMFLOAT3 cameraPos;
    float specularPower;
};

// Material constant buffer for Phong shading
struct alignas(16) MaterialBuffer {
    XMFLOAT3 diffuseColor;
    float metallic;
    XMFLOAT3 specularColor;
    float roughness;
    float shininess;
    float reflectance;
    float emissive;
    float padding1;
    XMFLOAT3 emissiveColor;
    float padding2;
};

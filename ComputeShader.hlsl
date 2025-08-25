/**
 * @file ComputeShader.hlsl
 * @brief Multi-type particle physics simulation compute shader
 * 
 * This compute shader implements a comprehensive particle physics system supporting
 * multiple particle types with configurable interactions. It uses a force-centric
 * architecture where different force models can be combined to create complex
 * particle behaviors.
 * 
 * Key features:
 * - Multi-type particle system with configurable type properties
 * - Force-based physics with support for 8+ different force types
 * - Flexible interaction rules between particle types  
 * - Environmental forces affecting specific particle types
 * - Boundary collision handling with configurable restitution
 * - Verlet integration for stable simulation
 * - Optimized GPU parallel processing
 * 
 * The shader processes particles in parallel threads, calculating forces,
 * updating positions using Verlet integration, and handling boundary collisions.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

// Force type enumeration - defines available force models
#define FORCE_NONE 0              ///< No force applied
#define FORCE_SPRING 1            ///< Hooke's law spring force (linear restoring)
#define FORCE_LENNARD_JONES 2     ///< Lennard-Jones potential (molecular interactions)
#define FORCE_GRAVITATIONAL 3     ///< Newtonian gravitational attraction
#define FORCE_ELECTROMAGNETIC 4   ///< Coulomb electrostatic force
#define FORCE_VISCOUS 5          ///< Velocity-dependent damping force
#define FORCE_CONSTANT_FIELD 6   ///< Uniform field force (e.g., gravity, electric)
#define FORCE_RADIAL_FIELD 7     ///< Radial field emanating from a point
#define FORCE_VORTEX_FIELD 8     ///< Swirling vortex field for fluid-like motion

/**
 * @struct Particle
 * @brief Core particle data structure containing all physical properties
 * 
 * Represents a single particle in the simulation with position, velocity,
 * acceleration, and physical properties. Uses Verlet integration scheme
 * where oldPosition stores the previous frame's position for integration.
 */
struct Particle
{
    float3 position;          ///< Current world position (x, y, z)
    float3 oldPosition;       ///< Previous frame position for Verlet integration
    float3 velocity;          ///< Current velocity vector (m/s)
    float3 acceleration;      ///< Net acceleration from all forces (m/s²)
    
    // Physical type and properties
    uint typeId;              ///< Particle type identifier for interaction rules
    float mass;               ///< Particle mass (kg) - affects force response
    float radius;             ///< Particle collision radius (m)
    float charge;             ///< Electric charge (C) for electromagnetic forces
    
    // Simulation state variables
    float temperature;        ///< Particle temperature for thermal effects
    float age;               ///< Time since particle creation (s)
    uint collisionCount;     ///< Number of boundary collisions (statistics)
    uint flags;              ///< Bit flags for particle state and special conditions
};

/**
 * @struct ParticleType
 * @brief Defines physical properties for a class of particles
 * 
 * Template for particle types, defining default physical properties
 * that can be assigned to particles of this type.
 */
struct ParticleType
{
    uint id;                 ///< Unique type identifier
    float mass;              ///< Default mass for particles of this type (kg)
    float radius;            ///< Default radius for particles of this type (m)
    float charge;            ///< Default electric charge for particles of this type (C)
    float padding;           ///< Padding for alignment
};

/**
 * @struct ForceDescriptor
 * @brief Configurable force model with flexible parameters
 * 
 * Defines a single force interaction with type-specific parameters.
 * The parameters array allows different force types to store their
 * specific configuration data in a unified structure.
 */
struct ForceDescriptor
{
    uint type;               ///< Force type (see FORCE_* defines)
    float strength;          ///< Base strength/magnitude of the force
    float range;             ///< Maximum effective range of the force (m)
    float padding;           ///< Padding for alignment
    
    // Flexible parameter array for different force types:
    // Spring: [restLength, stiffness, damping, ...]
    // LJ: [sigma, epsilon, cutoff, ...]
    // Gravity: [softening, ...]
    // EM: [permittivity, ...]
    float parameters[8];     ///< Type-specific force parameters
};

/**
 * @struct InteractionRule
 * @brief Defines how two particle types interact with each other
 * 
 * Specifies which forces are active between pairs of particle types
 * and provides global scaling factors for the interaction strength.
 */
struct InteractionRule
{
    uint typeA;              ///< First particle type ID
    uint typeB;              ///< Second particle type ID  
    uint enabled;            ///< Whether this interaction is active (0/1)
    uint numForces;          ///< Number of forces in this interaction
    
    float globalStrengthMultiplier; ///< Global scaling factor for all forces
    float maxInteractionRange;      ///< Maximum range for this interaction (m)
    float padding[2];               ///< Padding for alignment
};

/**
 * @struct EnvironmentalForce
 * @brief Global forces that act on particles based on their type
 * 
 * Represents environmental influences like gravity, electric fields,
 * or fluid drag that affect particles without requiring particle pairs.
 */
struct EnvironmentalForce
{
    uint targetTypeId;       ///< Target particle type (0xFFFFFFFF = all types)
    ForceDescriptor force;   ///< Force configuration and parameters
    uint enabled;            ///< Whether this environmental force is active
    float padding[3];        ///< Padding for alignment
};

// GPU Buffer Bindings - Input and output data for the compute shader
StructuredBuffer<Particle> particlesIn : register(t0);              ///< Input particle data (read-only)
RWStructuredBuffer<Particle> particlesOut : register(u0);           ///< Output particle data (read-write)
StructuredBuffer<ParticleType> particleTypes : register(t1);        ///< Particle type definitions (read-only)
StructuredBuffer<InteractionRule> interactionRules : register(t2);  ///< Type interaction rules (read-only)
StructuredBuffer<ForceDescriptor> forceDescriptors : register(t3);  ///< Force definitions from interactions (read-only)
StructuredBuffer<EnvironmentalForce> environmentalForces : register(t4); ///< Global environmental forces (read-only)

/**
 * @brief Simulation parameters constant buffer
 * 
 * Contains global simulation parameters that remain constant during
 * a single simulation step but can change between frames.
 */
cbuffer SimulationConstants : register(b0)
{
    float deltaTime;          ///< Time step for this simulation frame (s)
    float globalDamping;      ///< Global damping coefficient [0,1]
    float2 padding1;          ///< Padding for alignment
    
    float3 boundaryMin;       ///< Minimum boundary coordinates (x, y, z)
    float boundaryRestitution; ///< Energy retention on boundary collision [0,1]
    
    float3 boundaryMax;       ///< Maximum boundary coordinates (x, y, z)
    uint numParticleTypes;    ///< Total number of particle types
    
    uint numInteractionRules;        ///< Total number of interaction rules
    uint maxInteractionsPerParticle; ///< Maximum interactions per particle (optimization)
    float spatialGridSize;           ///< Grid size for spatial partitioning (m)
    uint numEnvironmentalForces;     ///< Number of active environmental forces
};

//=============================================================================
// Force Calculation Functions
//=============================================================================

/**
 * @brief Calculates spring force between two particles using Hooke's law
 * 
 * Implements a linear spring force with optional damping. The force magnitude
 * is proportional to the displacement from the rest length.
 * 
 * Force equation: F = -k(r - r0) where:
 * - k = spring stiffness (stored in parameters[0])
 * - r = current distance
 * - r0 = rest length (stored in parameters[2])
 * 
 * @param displacement Vector from particle A to particle B
 * @param distance Distance between particles
 * @param force Force descriptor with spring parameters
 * @return Spring force vector to apply to particle A
 */
float3 CalculateSpringForce(float3 displacement, float distance, ForceDescriptor force)
{
    // Prevent division by zero and singularities
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float stiffness = force.parameters[0];  // Spring constant (N/m)
    float dampingCoeff = force.parameters[1]; // Damping coefficient
    float restLength = force.parameters[2];   // Rest length (m)
    
    float3 direction = displacement / distance;
    float extension = distance - restLength;  // Positive = stretched, negative = compressed
    float forceStrength = -stiffness * extension; // Hooke's law
    
    return direction * forceStrength * force.strength;
}

/**
 * @brief Calculates Lennard-Jones force for molecular interactions
 * 
 * Implements the 6-12 Lennard-Jones potential commonly used in molecular
 * dynamics simulations. Provides both attractive and repulsive forces.
 * 
 * Potential: U(r) = 4ε[(σ/r)^12 - (σ/r)^6]
 * Force: F(r) = 24ε[(σ/r)^6][2(σ/r)^6 - 1]/r
 * 
 * @param displacement Vector from particle A to particle B
 * @param distance Distance between particles
 * @param force Force descriptor with LJ parameters (epsilon, sigma)
 * @return Lennard-Jones force vector to apply to particle A
 */
float3 CalculateLennardJonesForce(float3 displacement, float distance, ForceDescriptor force)
{
    // Prevent division by zero and numerical instabilities
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float epsilon = force.parameters[0]; // Depth of potential well (J)
    float sigma = force.parameters[1];   // Distance at zero potential (m)
    
    // Calculate normalized distance ratios
    float r6 = pow(sigma / distance, 6);
    float r12 = r6 * r6;
    
    // Lennard-Jones force magnitude
    float forceMagnitude = 24.0f * epsilon * (2.0f * r12 - r6) / distance;
    
    return normalize(displacement) * forceMagnitude * force.strength;
}

/**
 * @brief Calculates gravitational force between two particles
 * 
 * Implements Newton's law of universal gravitation with optional softening
 * to prevent numerical instabilities at very small distances.
 * 
 * Force equation: F = G * m1 * m2 / r²
 * where G is the gravitational constant stored in parameters[0]
 * 
 * @param displacement Vector from particle A to particle B
 * @param distance Distance between particles
 * @param mass1 Mass of first particle (kg)
 * @param mass2 Mass of second particle (kg)
 * @param force Force descriptor with gravitational parameters
 * @return Gravitational force vector to apply to particle A (attractive)
 */
float3 CalculateGravitationalForce(float3 displacement, float distance, float mass1, float mass2, ForceDescriptor force)
{
    // Prevent division by zero and singularities
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float G = force.parameters[0];        // Gravitational constant
    float softening = force.parameters[1]; // Softening parameter (optional)
    
    // Add softening to prevent numerical instabilities at small distances
    float effectiveDistance = distance + softening;
    float forceMagnitude = G * mass1 * mass2 / (effectiveDistance * effectiveDistance);
    
    // Force points from A toward B (attractive)
    return normalize(-displacement) * forceMagnitude * force.strength;
}

/**
 * @brief Calculates electromagnetic (Coulomb) force between charged particles
 * 
 * Implements Coulomb's law for electrostatic interactions between charged
 * particles. The force can be attractive (opposite charges) or repulsive
 * (same charges).
 * 
 * Force equation: F = k * q1 * q2 / r²
 * where k is Coulomb's constant stored in parameters[0]
 * 
 * @param displacement Vector from particle A to particle B
 * @param distance Distance between particles
 * @param charge1 Charge of first particle (C)
 * @param charge2 Charge of second particle (C)
 * @param force Force descriptor with electromagnetic parameters
 * @return Electromagnetic force vector to apply to particle A
 */
float3 CalculateElectromagneticForce(float3 displacement, float distance, float charge1, float charge2, ForceDescriptor force)
{
    // Prevent division by zero and handle neutral particles
    if (distance < 0.0001f || abs(charge1) < 0.0001f || abs(charge2) < 0.0001f) return float3(0, 0, 0);
    
    float k = force.parameters[0];        // Coulomb's constant (N⋅m²/C²)
    float softening = force.parameters[1]; // Softening parameter (optional)
    
    // Add softening to prevent numerical instabilities
    float effectiveDistance = distance + softening;
    float forceMagnitude = k * charge1 * charge2 / (effectiveDistance * effectiveDistance);
    
    // Positive product = repulsive, negative product = attractive
    return normalize(displacement) * forceMagnitude * force.strength;
}

/**
 * @brief Calculates viscous drag force proportional to velocity
 * 
 * Implements velocity-dependent drag force commonly used to simulate
 * fluid resistance or air drag. The force opposes motion and is
 * proportional to velocity (linear drag) or velocity squared.
 * 
 * Force equation: F = -b * v (linear) or F = -c * v * |v| (quadratic)
 * 
 * @param velocity Current particle velocity
 * @param radius Particle radius for drag calculation
 * @param force Force descriptor with drag parameters
 * @return Viscous drag force vector opposing motion
 */
float3 CalculateViscousForce(float3 velocity, float radius, ForceDescriptor force)
{
    float viscosityCoeff = force.parameters[0]; // Viscosity coefficient (Pa⋅s)
    float dragCoeff = force.parameters[1];      // Additional drag coefficient
    
    // Stokes' law for spherical particles: F = 6πηrv
    float dragMagnitude = 6.0f * 3.14159f * viscosityCoeff * radius;
    
    // Apply drag force opposing velocity
    return -velocity * dragMagnitude * dragCoeff * force.strength;
}

/**
 * @brief Calculates constant field force (e.g., gravity, electric field)
 * 
 * Applies a uniform force field that affects particles based on their
 * properties. Can simulate gravity, uniform electric fields, or other
 * constant acceleration fields.
 * 
 * @param force Force descriptor containing field vector and mass scaling
 * @param mass Particle mass for mass-dependent forces
 * @return Constant field force vector
 */
float3 CalculateConstantFieldForce(ForceDescriptor force, float mass)
{
    // Field vector components (x, y, z acceleration)
    float3 fieldVector = float3(force.parameters[0], force.parameters[1], force.parameters[2]);
    float massMultiplier = force.parameters[3]; // How much force scales with mass
    
    // F = ma, with optional mass-dependent scaling
    return fieldVector * force.strength * (1.0f + massMultiplier * mass);
}

/**
 * @brief Calculates radial field force emanating from a point source
 * 
 * Creates a centrally symmetric force field that can be either attractive
 * or repulsive, following an inverse square law similar to gravity or
 * electrostatic fields.
 * 
 * @param position Current particle position
 * @param force Force descriptor with center position and field parameters
 * @return Radial field force vector
 */
float3 CalculateRadialFieldForce(float3 position, ForceDescriptor force)
{
    // Field center position
    float3 center = float3(force.parameters[0], force.parameters[1], force.parameters[2]);
    float fieldStrength = force.parameters[3]; // Field strength coefficient  
    float isRepulsive = force.parameters[4];   // > 0.5 = repulsive, < 0.5 = attractive
    
    float3 displacement = position - center;
    float distance = length(displacement);
    
    // Prevent singularity at field center
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float3 direction = displacement / distance;
    float forceMagnitude = fieldStrength / (distance * distance); // Inverse square law
    
    return direction * forceMagnitude * force.strength * (isRepulsive > 0.5f ? 1.0f : -1.0f);
}

/**
 * @brief Calculates vortex field force for swirling fluid-like motion
 * 
 * Creates a swirling force field around an axis, useful for simulating
 * whirlpools, tornadoes, or other rotational fluid phenomena.
 * 
 * @param position Current particle position
 * @param velocity Current particle velocity (for velocity-dependent effects)
 * @param force Force descriptor with vortex center, axis, and strength
 * @return Vortex force vector tangent to the rotation
 */
float3 CalculateVortexFieldForce(float3 position, float3 velocity, ForceDescriptor force)
{
    // Vortex center and rotation axis
    float3 center = float3(force.parameters[0], force.parameters[1], force.parameters[2]);
    float3 axis = normalize(float3(force.parameters[3], force.parameters[4], force.parameters[5]));
    float strength = force.parameters[6]; // Vortex strength
    
    // Project position to plane perpendicular to vortex axis
    float3 toParticle = position - center;
    float3 radialComponent = toParticle - dot(toParticle, axis) * axis;
    
    // Calculate tangent direction for swirling motion
    float3 tangent = cross(axis, radialComponent);
    
    return normalize(tangent) * strength * force.strength;
}

//=============================================================================
// Force Application and Integration
//=============================================================================

/**
 * @brief Applies a single force descriptor to a particle
 * 
 * Dispatches to the appropriate force calculation function based on the
 * force type. Handles both pair-wise forces (requiring another particle)
 * and single-particle forces (environmental).
 * 
 * @param particle The particle to apply force to
 * @param force The force descriptor defining the force type and parameters
 * @param otherParticle Second particle for pair-wise forces (optional)
 * @param hasOther Whether otherParticle is valid for pair-wise forces
 * @return Calculated force vector to apply to the particle
 */
float3 ApplyForceDescriptor(Particle particle, ForceDescriptor force, Particle otherParticle = (Particle)0, bool hasOther = false)
{
    switch (force.type)
    {
        case FORCE_SPRING:
            if (hasOther) {
                float3 displacement = otherParticle.position - particle.position;
                float distance = length(displacement);
                return CalculateSpringForce(displacement, distance, force);
            }
            break;
            
        case FORCE_LENNARD_JONES:
            if (hasOther) {
                float3 displacement = otherParticle.position - particle.position;
                float distance = length(displacement);
                return CalculateLennardJonesForce(displacement, distance, force);
            }
            break;
            
        case FORCE_GRAVITATIONAL:
            if (hasOther) {
                float3 displacement = otherParticle.position - particle.position;
                float distance = length(displacement);
                return CalculateGravitationalForce(displacement, distance, particle.mass, otherParticle.mass, force);
            }
            break;
            
        case FORCE_ELECTROMAGNETIC:
            if (hasOther) {
                float3 displacement = otherParticle.position - particle.position;
                float distance = length(displacement);
                return CalculateElectromagneticForce(displacement, distance, particle.charge, otherParticle.charge, force);
            }
            break;
            
        // Single-particle forces (no pair interaction required)
        case FORCE_VISCOUS:
            return CalculateViscousForce(particle.velocity, particle.radius, force);
            
        case FORCE_CONSTANT_FIELD:
            return CalculateConstantFieldForce(force, particle.mass);
            
        case FORCE_RADIAL_FIELD:
            return CalculateRadialFieldForce(particle.position, force);
            
        case FORCE_VORTEX_FIELD:
            return CalculateVortexFieldForce(particle.position, particle.velocity, force);
            
        default:
            break;
    }
    
    return float3(0, 0, 0); // No force applied for unrecognized types
}

/**
 * @brief Handles boundary collisions with configurable restitution
 * 
 * Checks if a particle has collided with simulation boundaries and updates
 * its velocity accordingly. Uses coefficient of restitution to control
 * energy loss during collisions.
 * 
 * @param position Current particle position
 * @param velocity Current particle velocity
 * @param radius Particle collision radius
 * @return Updated velocity after boundary collision handling
 */
float3 HandleBoundaryCollisions(float3 position, float3 velocity, float radius)
{
    float3 newVelocity = velocity;
    
    // Check X-axis boundaries
    if (position.x - radius <= boundaryMin.x)
    {
        newVelocity.x = abs(newVelocity.x) * boundaryRestitution; // Bounce right
    }
    else if (position.x + radius >= boundaryMax.x)
    {
        newVelocity.x = -abs(newVelocity.x) * boundaryRestitution; // Bounce left
    }
    
    // Check Y-axis boundaries
    if (position.y - radius <= boundaryMin.y)
    {
        newVelocity.y = abs(newVelocity.y) * boundaryRestitution; // Bounce up
    }
    else if (position.y + radius >= boundaryMax.y)
    {
        newVelocity.y = -abs(newVelocity.y) * boundaryRestitution; // Bounce down
    }
    
    // Check Z-axis boundaries
    if (position.z - radius <= boundaryMin.z)
    {
        newVelocity.z = abs(newVelocity.z) * boundaryRestitution; // Bounce forward
    }
    else if (position.z + radius >= boundaryMax.z)
    {
        newVelocity.z = -abs(newVelocity.z) * boundaryRestitution; // Bounce backward
    }
    
    return newVelocity;
}

//=============================================================================
// Main Compute Shader Entry Point
//=============================================================================

/**
 * @brief Main compute shader function for particle physics simulation
 * 
 * This is the entry point for the GPU-accelerated particle simulation.
 * Each thread processes one particle, calculating all forces acting on it,
 * updating its state using Verlet integration, and handling boundary collisions.
 * 
 * The simulation process for each particle:
 * 1. Apply environmental forces (gravity, fields, drag)
 * 2. Calculate pair-wise interaction forces with other particles
 * 3. Apply global damping to reduce system energy
 * 4. Update position and velocity using Verlet integration
 * 5. Handle boundary collisions with energy loss
 * 6. Update particle age and collision statistics
 * 
 * Thread group size: 64 threads per group for optimal GPU utilization
 * 
 * @param dispatchID Thread dispatch identifier containing particle index
 */
[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    uint index = dispatchID.x;
    
    // Get total number of particles and ensure we're within bounds
    uint numParticles, stride;
    particlesIn.GetDimensions(numParticles, stride);
    
    if (index >= numParticles) return;
    
    // Load current particle data from input buffer
    Particle particle = particlesIn[index];
    
    // Initialize force accumulator for this simulation step
    float3 totalForce = float3(0, 0, 0);
    uint interactionCount = 0;
    
    //-----------------------------------------------------------------------
    // Phase 1: Apply Environmental Forces
    //-----------------------------------------------------------------------
    for (uint envIdx = 0; envIdx < numEnvironmentalForces; envIdx++)
    {
        EnvironmentalForce envForce = environmentalForces[envIdx];
        
        if (!envForce.enabled) continue;
        
        // Check if this environmental force applies to this particle type
        if (envForce.targetTypeId != 0xFFFFFFFF && envForce.targetTypeId != particle.typeId)
            continue;
            
        totalForce += ApplyForceDescriptor(particle, envForce.force, (Particle)0, false);
    }
    
    //-----------------------------------------------------------------------  
    // Phase 2: Calculate Pair-wise Interaction Forces
    //-----------------------------------------------------------------------
    uint currentForceIdx = 0;
    
    for (uint ruleIdx = 0; ruleIdx < numInteractionRules && interactionCount < maxInteractionsPerParticle; ruleIdx++)
    {
        InteractionRule rule = interactionRules[ruleIdx];
        
        if (!rule.enabled) continue;
        
        // Check if this interaction rule applies to current particle type
        bool appliesToThisType = (rule.typeA == particle.typeId) || (rule.typeB == particle.typeId);
        if (!appliesToThisType) continue;
        
        // Test interactions with all other particles
        for (uint otherIdx = 0; otherIdx < numParticles && interactionCount < maxInteractionsPerParticle; otherIdx++)
        {
            if (otherIdx == index) continue; // Skip self-interaction
            
            Particle otherParticle = particlesIn[otherIdx];
            
            // Verify that this particle pair matches the interaction rule
            bool validPair = ((rule.typeA == particle.typeId && rule.typeB == otherParticle.typeId) ||
                             (rule.typeB == particle.typeId && rule.typeA == otherParticle.typeId));
            
            if (!validPair) continue;
            
            float3 displacement = otherParticle.position - particle.position;
            float distance = length(displacement);
            
            // Check if particles are within interaction range
            if (distance > rule.maxInteractionRange) continue;
            
            // Apply all forces defined for this interaction rule
            for (uint forceIdx = 0; forceIdx < rule.numForces && currentForceIdx < 1024; forceIdx++, currentForceIdx++)
            {
                ForceDescriptor force = forceDescriptors[currentForceIdx];
                
                // Check if within this specific force's effective range
                if (distance > force.range) continue;
                
                float3 forceContribution = ApplyForceDescriptor(particle, force, otherParticle, true);
                totalForce += forceContribution * rule.globalStrengthMultiplier;
            }
            
            interactionCount++;
        }
    }
    
    //-----------------------------------------------------------------------
    // Phase 3: Apply Global Damping and Integration
    //-----------------------------------------------------------------------
    
    // Apply global velocity damping to prevent energy buildup
    totalForce -= particle.velocity * globalDamping;
    
    // Calculate acceleration from net force (F = ma)
    particle.acceleration = totalForce / particle.mass;
    
    // Verlet integration: x(t+dt) = 2x(t) - x(t-dt) + a(t)dt²
    float3 newPosition = 2.0f * particle.position - particle.oldPosition + particle.acceleration * deltaTime * deltaTime;
    particle.velocity = (newPosition - particle.position) / deltaTime;
    
    //-----------------------------------------------------------------------
    // Phase 4: Boundary Collision Handling and Position Updates
    //-----------------------------------------------------------------------
    
    // Handle collisions with simulation boundaries (with energy loss)
    particle.velocity = HandleBoundaryCollisions(newPosition, particle.velocity, particle.radius);
    newPosition = particle.position + particle.velocity * deltaTime;
    
    // Ensure particle stays within boundaries (hard constraint)
    newPosition = clamp(newPosition, boundaryMin + particle.radius, boundaryMax - particle.radius);
    
    //-----------------------------------------------------------------------
    // Phase 5: Update Particle State and Output
    //-----------------------------------------------------------------------
    
    // Store current position as old position for next Verlet step
    particle.oldPosition = particle.position;
    particle.position = newPosition;
    
    // Update particle age for time-dependent effects
    particle.age += deltaTime;
    
    // Write updated particle data to output buffer
    particlesOut[index] = particle;
}

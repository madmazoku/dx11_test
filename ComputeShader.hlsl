// Clean Multi-Type Particle Simulation Compute Shader
// Modern architecture with force-centric design

// Force type enumeration - clean and focused
#define FORCE_NONE 0
#define FORCE_SPRING 1
#define FORCE_LENNARD_JONES 2
#define FORCE_GRAVITATIONAL 3
#define FORCE_ELECTROMAGNETIC 4
#define FORCE_VISCOUS 5
#define FORCE_CONSTANT_FIELD 6
#define FORCE_RADIAL_FIELD 7
#define FORCE_VORTEX_FIELD 8

// Particle structure - only essential properties
struct Particle
{
    float3 position;
    float3 oldPosition;
    float3 velocity;
    float3 acceleration;
    
    // Type and essential physical properties
    uint typeId;
    float mass;
    float radius;
    float charge;
    
    // State variables
    float temperature;
    float age;
    uint collisionCount;
    uint flags;
};

// Clean particle type information - only essentials
struct ParticleType
{
    uint id;
    float mass;
    float radius;
    float charge;
    float padding;
};

// Clean force descriptor with flexible parameters
struct ForceDescriptor
{
    uint type;                  // ForceType
    float strength;
    float range;
    float padding;
    
    // Flexible parameter array for all force types
    float parameters[8];        // Type-specific parameters
};

// Clean interaction rule - supports multiple forces
struct InteractionRule
{
    uint typeA;
    uint typeB;
    uint enabled;
    uint numForces;             // Number of forces in this interaction
    
    float globalStrengthMultiplier;
    float maxInteractionRange;
    float padding[2];
};

// Environmental force acting on specific types
struct EnvironmentalForce
{
    uint targetTypeId;          // 0xFFFFFFFF = all types
    ForceDescriptor force;
    uint enabled;
    float padding[3];
};

// Input/Output buffers - clean structure
StructuredBuffer<Particle> particlesIn : register(t0);
RWStructuredBuffer<Particle> particlesOut : register(u0);
StructuredBuffer<ParticleType> particleTypes : register(t1);
StructuredBuffer<InteractionRule> interactionRules : register(t2);
StructuredBuffer<ForceDescriptor> forceDescriptors : register(t3);  // All forces from all interactions
StructuredBuffer<EnvironmentalForce> environmentalForces : register(t4);

// Clean simulation constants
cbuffer SimulationConstants : register(b0)
{
    float deltaTime;
    float globalDamping;
    float2 padding1;
    
    float3 boundaryMin;
    float boundaryRestitution;
    
    float3 boundaryMax;
    uint numParticleTypes;
    
    uint numInteractionRules;
    uint maxInteractionsPerParticle;
    float spatialGridSize;
    uint numEnvironmentalForces;
};


// Clean force calculation functions

float3 CalculateSpringForce(float3 displacement, float distance, ForceDescriptor force)
{
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float stiffness = force.parameters[0];
    float dampingCoeff = force.parameters[1];
    float restLength = force.parameters[2];
    
    float3 direction = displacement / distance;
    float extension = distance - restLength;
    float forceStrength = -stiffness * extension;
    
    return direction * forceStrength * force.strength;
}

float3 CalculateLennardJonesForce(float3 displacement, float distance, ForceDescriptor force)
{
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float epsilon = force.parameters[0];
    float sigma = force.parameters[1];
    
    float r6 = pow(sigma / distance, 6);
    float r12 = r6 * r6;
    
    float forceMagnitude = 24.0f * epsilon * (2.0f * r12 - r6) / distance;
    
    return normalize(displacement) * forceMagnitude * force.strength;
}

float3 CalculateGravitationalForce(float3 displacement, float distance, float mass1, float mass2, ForceDescriptor force)
{
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float G = force.parameters[0];
    float forceMagnitude = G * mass1 * mass2 / (distance * distance);
    
    return normalize(-displacement) * forceMagnitude * force.strength;
}

float3 CalculateElectromagneticForce(float3 displacement, float distance, float charge1, float charge2, ForceDescriptor force)
{
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float k = force.parameters[0];
    float forceMagnitude = k * charge1 * charge2 / (distance * distance);
    
    return normalize(displacement) * forceMagnitude * force.strength;
}

float3 CalculateViscousForce(float3 velocity, float radius, ForceDescriptor force)
{
    float viscosityCoeff = force.parameters[0];
    float dragCoeff = force.parameters[1];
    
    float dragMagnitude = 6.0f * 3.14159f * viscosityCoeff * radius;
    
    return -velocity * dragMagnitude * dragCoeff * force.strength;
}

float3 CalculateConstantFieldForce(ForceDescriptor force, float mass)
{
    float3 fieldVector = float3(force.parameters[0], force.parameters[1], force.parameters[2]);
    float massMultiplier = force.parameters[3];
    
    return fieldVector * force.strength * (1.0f + massMultiplier * mass);
}

float3 CalculateRadialFieldForce(float3 position, ForceDescriptor force)
{
    float3 center = float3(force.parameters[0], force.parameters[1], force.parameters[2]);
    float fieldStrength = force.parameters[3];
    float isRepulsive = force.parameters[4];
    
    float3 displacement = position - center;
    float distance = length(displacement);
    
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float3 direction = displacement / distance;
    float forceMagnitude = fieldStrength / (distance * distance);
    
    return direction * forceMagnitude * force.strength * (isRepulsive > 0.5f ? 1.0f : -1.0f);
}

float3 CalculateVortexFieldForce(float3 position, float3 velocity, ForceDescriptor force)
{
    float3 center = float3(force.parameters[0], force.parameters[1], force.parameters[2]);
    float3 axis = normalize(float3(force.parameters[3], force.parameters[4], force.parameters[5]));
    float strength = force.parameters[6];
    
    float3 toParticle = position - center;
    float3 radialComponent = toParticle - dot(toParticle, axis) * axis;
    float3 tangent = cross(axis, radialComponent);
    
    return normalize(tangent) * strength * force.strength;
}

// Apply a single force descriptor
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
            
        case FORCE_VISCOUS:
            return CalculateViscousForce(particle.velocity, particle.radius, force);
            
        case FORCE_CONSTANT_FIELD:
            return CalculateConstantFieldForce(force, particle.mass);
            
        case FORCE_RADIAL_FIELD:
            return CalculateRadialFieldForce(particle.position, force);
            
        case FORCE_VORTEX_FIELD:
            return CalculateVortexFieldForce(particle.position, particle.velocity, force);
    }
    
    return float3(0, 0, 0);
}

// Boundary collision handling
float3 HandleBoundaryCollisions(float3 position, float3 velocity, float radius)
{
    float3 newVelocity = velocity;
    
    // Check each boundary
    if (position.x - radius <= boundaryMin.x)
    {
        newVelocity.x = abs(newVelocity.x) * boundaryRestitution;
    }
    else if (position.x + radius >= boundaryMax.x)
    {
        newVelocity.x = -abs(newVelocity.x) * boundaryRestitution;
    }
    
    if (position.y - radius <= boundaryMin.y)
    {
        newVelocity.y = abs(newVelocity.y) * boundaryRestitution;
    }
    else if (position.y + radius >= boundaryMax.y)
    {
        newVelocity.y = -abs(newVelocity.y) * boundaryRestitution;
    }
    
    if (position.z - radius <= boundaryMin.z)
    {
        newVelocity.z = abs(newVelocity.z) * boundaryRestitution;
    }
    else if (position.z + radius >= boundaryMax.z)
    {
        newVelocity.z = -abs(newVelocity.z) * boundaryRestitution;
    }
    
    return newVelocity;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    uint index = dispatchID.x;
    
    uint numParticles, stride;
    particlesIn.GetDimensions(numParticles, stride);
    
    if (index >= numParticles) return;
    
    Particle particle = particlesIn[index];
    
    // Initialize total force
    float3 totalForce = float3(0, 0, 0);
    uint interactionCount = 0;
    
    // Apply environmental forces
    for (uint envIdx = 0; envIdx < numEnvironmentalForces; envIdx++)
    {
        EnvironmentalForce envForce = environmentalForces[envIdx];
        
        if (!envForce.enabled) continue;
        
        // Check if this force applies to this particle type
        if (envForce.targetTypeId != 0xFFFFFFFF && envForce.targetTypeId != particle.typeId)
            continue;
            
        totalForce += ApplyForceDescriptor(particle, envForce.force, (Particle)0, false);
    }
    
    // Calculate interaction forces with other particles
    uint currentForceIdx = 0;
    
    for (uint ruleIdx = 0; ruleIdx < numInteractionRules && interactionCount < maxInteractionsPerParticle; ruleIdx++)
    {
        InteractionRule rule = interactionRules[ruleIdx];
        
        if (!rule.enabled) continue;
        
        // Check if this rule applies to current particle type
        bool appliesToThisType = (rule.typeA == particle.typeId) || (rule.typeB == particle.typeId);
        if (!appliesToThisType) continue;
        
        // Find other particles to interact with
        for (uint otherIdx = 0; otherIdx < numParticles && interactionCount < maxInteractionsPerParticle; otherIdx++)
        {
            if (otherIdx == index) continue;
            
            Particle otherParticle = particlesIn[otherIdx];
            
            // Check if interaction rule applies to this pair
            bool validPair = ((rule.typeA == particle.typeId && rule.typeB == otherParticle.typeId) ||
                             (rule.typeB == particle.typeId && rule.typeA == otherParticle.typeId));
            
            if (!validPair) continue;
            
            float3 displacement = otherParticle.position - particle.position;
            float distance = length(displacement);
            
            // Check if within maximum interaction range
            if (distance > rule.maxInteractionRange) continue;
            
            // Apply all forces for this interaction rule
            for (uint forceIdx = 0; forceIdx < rule.numForces && currentForceIdx < 1024; forceIdx++, currentForceIdx++)
            {
                ForceDescriptor force = forceDescriptors[currentForceIdx];
                
                // Check if within this specific force's range
                if (distance > force.range) continue;
                
                float3 forceContribution = ApplyForceDescriptor(particle, force, otherParticle, true);
                totalForce += forceContribution * rule.globalStrengthMultiplier;
            }
            
            interactionCount++;
        }
    }
    
    // Apply damping
    totalForce -= particle.velocity * globalDamping;
    
    // Update particle using Verlet integration
    particle.acceleration = totalForce / particle.mass;
    
    float3 newPosition = 2.0f * particle.position - particle.oldPosition + particle.acceleration * deltaTime * deltaTime;
    particle.velocity = (newPosition - particle.position) / deltaTime;
    
    // Handle boundary collisions
    particle.velocity = HandleBoundaryCollisions(newPosition, particle.velocity, particle.radius);
    newPosition = particle.position + particle.velocity * deltaTime;
    
    // Clamp position to boundaries
    newPosition = clamp(newPosition, boundaryMin + particle.radius, boundaryMax - particle.radius);
    
    // Update particle
    particle.oldPosition = particle.position;
    particle.position = newPosition;
    particle.age += deltaTime;
    
    particlesOut[index] = particle;
}

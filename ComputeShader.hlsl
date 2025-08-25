struct Particle
{
    float3 position;
    float3 oldPosition;
    float3 acceleration;
    float padding;
};

StructuredBuffer<Particle> particlesIn : register(t0);
RWStructuredBuffer<Particle> particlesOut : register(u0);

// Simulation constants from constant buffer
cbuffer SimulationConstants : register(b0)
{
    float springConstant;
    float restLength;
    float timeStep;
    float damping;
    float3 gravity;
    float padding1;
    float3 boundaryMin;
    float padding2;
    float3 boundaryMax;
    float padding3;
};

float3 CalculateSpringForce(float3 displacement, float distance)
{
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float3 direction = displacement / distance;
    float force = springConstant * (distance - restLength);
    return direction * force;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    uint index = dispatchID.x;
    
    uint numParticles, stride;
    particlesIn.GetDimensions(numParticles, stride);
    
    if (index >= numParticles) return;
    
    Particle particle = particlesIn[index];
    
    // Calculate forces from other particles
    float3 totalForce = gravity; // Start with gravity
    
    for (uint i = 0; i < numParticles; i++)
    {
        if (i != index)
        {
            float3 displacement = particlesIn[i].position - particle.position;
            float distance = length(displacement);
            
            // Only interact with nearby particles to avoid expensive computations
            if (distance > 0.0001f && distance < 2.0f * restLength)
            {
                totalForce += CalculateSpringForce(displacement, distance);
            }
        }
    }
    
    // Verlet integration
    float3 newAcceleration = totalForce; // Assuming unit mass
    float3 newPosition = 2.0f * particle.position - particle.oldPosition + newAcceleration * timeStep * timeStep;
    
    // Apply damping
    newPosition = particle.position + (newPosition - particle.position) * damping;
    
    // Boundary constraints with bounce
    if (newPosition.x < boundaryMin.x) {
        newPosition.x = boundaryMin.x;
        newPosition = particle.position + (newPosition - particle.position) * 0.8f; // Bounce damping
    }
    if (newPosition.x > boundaryMax.x) {
        newPosition.x = boundaryMax.x;
        newPosition = particle.position + (newPosition - particle.position) * 0.8f;
    }
    if (newPosition.y < boundaryMin.y) {
        newPosition.y = boundaryMin.y;
        newPosition = particle.position + (newPosition - particle.position) * 0.8f;
    }
    if (newPosition.y > boundaryMax.y) {
        newPosition.y = boundaryMax.y;
        newPosition = particle.position + (newPosition - particle.position) * 0.8f;
    }
    if (newPosition.z < boundaryMin.z) {
        newPosition.z = boundaryMin.z;
        newPosition = particle.position + (newPosition - particle.position) * 0.8f;
    }
    if (newPosition.z > boundaryMax.z) {
        newPosition.z = boundaryMax.z;
        newPosition = particle.position + (newPosition - particle.position) * 0.8f;
    }
    
    // Update particle
    Particle newParticle;
    newParticle.oldPosition = particle.position;
    newParticle.position = newPosition;
    newParticle.acceleration = newAcceleration;
    newParticle.padding = 0.0f;
    
    particlesOut[index] = newParticle;
}

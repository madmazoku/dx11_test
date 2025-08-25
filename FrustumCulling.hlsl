// Frustum Culling Compute Shader
// Performs GPU-based frustum culling for particle rendering optimization

struct Particle {
    float3 position;
    float3 oldPosition;
    float3 velocity;
    float3 acceleration;
    
    uint typeId;
    float mass;
    float radius;
    float charge;
    
    float temperature;
    float age;
    uint collisionCount;
    uint flags;
};

struct CullingData {
    uint visible;          // 1 if visible, 0 if culled
    uint lodLevel;         // Level of detail (0 = highest, 3 = lowest)
    float distanceToCamera;
    uint padding;
};

// Input/Output buffers
StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<CullingData> cullingResults : register(u0);

// Frustum culling constants
cbuffer FrustumConstants : register(b0)
{
    float4 frustumPlanes[6];        // Left, Right, Top, Bottom, Near, Far
    float3 cameraPosition;
    float maxRenderDistance;
    
    float lodDistance1;             // High detail distance
    float lodDistance2;             // Medium detail distance  
    float lodDistance3;             // Low detail distance
    float globalScale;
    
    float minParticleRadius;
    float maxParticleRadius;
    uint enableDistanceCulling;
    uint enableLOD;
};

// Test if a sphere is inside or intersects the frustum
bool SphereInFrustum(float3 center, float radius)
{
    [unroll]
    for (uint i = 0; i < 6; i++)
    {
        float distance = dot(float4(center, 1.0f), frustumPlanes[i]);
        if (distance < -radius)
            return false;  // Sphere is completely outside this plane
    }
    return true;  // Sphere is inside or intersecting frustum
}

// Calculate LOD level based on distance
uint CalculateLODLevel(float distance)
{
    if (!enableLOD) return 0;
    
    if (distance <= lodDistance1) return 0;      // High detail
    if (distance <= lodDistance2) return 1;      // Medium detail  
    if (distance <= lodDistance3) return 2;      // Low detail
    return 3;                                    // Minimal detail
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    uint index = dispatchID.x;
    
    uint numParticles, stride;
    particles.GetDimensions(numParticles, stride);
    
    if (index >= numParticles)
        return;
    
    Particle particle = particles[index];
    CullingData result = (CullingData)0;
    
    // Calculate distance to camera
    float3 toCamera = particle.position - cameraPosition;
    float distanceToCamera = length(toCamera);
    result.distanceToCamera = distanceToCamera;
    
    // Distance culling
    if (enableDistanceCulling && distanceToCamera > maxRenderDistance)
    {
        result.visible = 0;
        result.lodLevel = 3;
        cullingResults[index] = result;
        return;
    }
    
    // Scale particle radius
    float scaledRadius = clamp(particle.radius * globalScale, 
                              minParticleRadius, maxParticleRadius);
    
    // Frustum culling - test if particle sphere is visible
    result.visible = SphereInFrustum(particle.position, scaledRadius) ? 1 : 0;
    
    // Calculate LOD level
    result.lodLevel = CalculateLODLevel(distanceToCamera);
    
    cullingResults[index] = result;
}

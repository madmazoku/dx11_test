/**
 * @file FrustumCulling.hlsl
 * @brief GPU-accelerated frustum culling compute shader for particle rendering optimization
 * 
 * This compute shader performs efficient frustum culling on particle systems to eliminate
 * particles that are outside the camera's view frustum. It also implements distance-based
 * culling and Level of Detail (LOD) selection to further optimize rendering performance.
 * 
 * Key features:
 * - 6-plane frustum culling using signed distance tests
 * - Distance-based culling with configurable maximum render distance
 * - 4-level LOD system based on distance to camera
 * - Per-particle visibility and LOD determination
 * - Optimized parallel processing for thousands of particles
 * 
 * The culling process determines which particles are visible and assigns appropriate
 * LOD levels, allowing the renderer to skip invisible particles and use simplified
 * geometry for distant particles.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

/**
 * @struct Particle
 * @brief Particle data structure for culling operations
 * 
 * Contains the essential particle information needed for frustum culling,
 * primarily position and radius for sphere-frustum intersection tests.
 */
struct Particle {
    float3 position;      ///< Current world position
    float3 oldPosition;   ///< Previous position (unused in culling)
    float3 velocity;      ///< Current velocity (unused in culling)
    float3 acceleration;  ///< Current acceleration (unused in culling)
    
    uint typeId;          ///< Particle type identifier (unused in culling)
    float mass;           ///< Particle mass (unused in culling)
    float radius;         ///< Particle radius for culling sphere tests
    float charge;         ///< Particle charge (unused in culling)
    
    float temperature;    ///< Particle temperature (unused in culling)
    float age;            ///< Particle age (unused in culling)
    uint collisionCount;  ///< Collision statistics (unused in culling)
    uint flags;           ///< Particle state flags (unused in culling)
};

/**
 * @struct CullingData
 * @brief Output structure containing culling results for each particle
 * 
 * Stores the visibility determination and LOD level for each particle,
 * used by the renderer to decide whether and how to draw each particle.
 */
struct CullingData {
    uint visible;          ///< 1 if particle is visible, 0 if culled
    uint lodLevel;         ///< Level of detail: 0=highest, 1=medium, 2=low, 3=culled
    float distanceToCamera; ///< Distance from particle to camera position
    uint padding;          ///< Padding for alignment
};

// GPU buffer bindings for input particles and output culling results
StructuredBuffer<Particle> particles : register(t0);           ///< Input particle data (read-only)
RWStructuredBuffer<CullingData> cullingResults : register(u0); ///< Output culling results (read-write)

/**
 * @brief Frustum culling parameters constant buffer
 * 
 * Contains the camera frustum planes, camera position, and culling parameters
 * that define the culling behavior and LOD thresholds.
 */
cbuffer FrustumConstants : register(b0)
{
    float4 frustumPlanes[6];    ///< Frustum planes: [0]=Left, [1]=Right, [2]=Top, [3]=Bottom, [4]=Near, [5]=Far
    float3 cameraPosition;      ///< Camera position in world space
    float maxRenderDistance;    ///< Maximum distance for rendering particles
    
    float lodDistance1;         ///< Distance threshold for high detail (LOD 0->1)
    float lodDistance2;         ///< Distance threshold for medium detail (LOD 1->2)  
    float lodDistance3;         ///< Distance threshold for low detail (LOD 2->3)
    float globalScale;          ///< Global scale factor for all distances
    
    float minParticleRadius;    ///< Minimum particle radius for culling
    float maxParticleRadius;    ///< Maximum particle radius for culling
    uint enableDistanceCulling; ///< Whether to enable distance-based culling
    uint enableLOD;             ///< Whether to enable LOD selection
};

/**
 * @brief Tests if a sphere is inside or intersects the view frustum
 * 
 * Performs sphere-frustum intersection test using the signed distance
 * from the sphere center to each frustum plane. A sphere is visible if
 * it's on the inside (or intersecting) all six frustum planes.
 * 
 * @param center Sphere center position in world space
 * @param radius Sphere radius for intersection testing
 * @return true if sphere is visible (inside or intersecting frustum)
 */
bool SphereInFrustum(float3 center, float radius)
{
    // Test sphere against all six frustum planes
    [unroll]
    for (uint i = 0; i < 6; i++)
    {
        // Calculate signed distance from sphere center to plane
        float distance = dot(float4(center, 1.0f), frustumPlanes[i]);
        
        // If sphere is completely behind/outside this plane, it's culled
        if (distance < -radius)
            return false;  // Sphere is completely outside this plane
    }
    return true;  // Sphere is inside or intersecting frustum
}

/**
 * @brief Calculates Level of Detail based on distance from camera
 * 
 * Determines the appropriate LOD level for a particle based on its distance
 * from the camera. Closer particles get higher detail, while distant particles
 * use simplified representations for better performance.
 * 
 * LOD Levels:
 * - LOD 0: High detail (close particles)
 * - LOD 1: Medium detail 
 * - LOD 2: Low detail
 * - LOD 3: Culled (too far to render)
 * 
 * @param distance Distance from particle to camera
 * @return LOD level (0-3), where 3 means culled
 */
uint CalculateLODLevel(float distance)
{
    // If LOD is disabled, always use highest detail
    if (!enableLOD) return 0;
    
    // Apply global scale factor to distances
    float scaledDistance = distance * globalScale;
    
    if (scaledDistance <= lodDistance1) return 0;      // High detail
    if (scaledDistance <= lodDistance2) return 1;      // Medium detail  
    if (scaledDistance <= lodDistance3) return 2;      // Low detail
    return 3;                                          // Too far - effectively culled
}

//=============================================================================
// Main Frustum Culling Compute Shader
//=============================================================================

/**
 * @brief Main compute shader entry point for frustum culling
 * 
 * This function processes each particle to determine:
 * 1. Whether it's within the camera's view frustum
 * 2. Whether it's within the maximum render distance
 * 3. What LOD level should be used based on distance
 * 
 * The results are stored in the cullingResults buffer for use by the renderer.
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
    particles.GetDimensions(numParticles, stride);
    
    if (index >= numParticles)
        return;
    
    // Load particle data and initialize result structure
    Particle particle = particles[index];
    CullingData result = (CullingData)0;
    
    // Calculate distance from particle to camera
    float3 toCamera = particle.position - cameraPosition;
    float distanceToCamera = length(toCamera);
    result.distanceToCamera = distanceToCamera;
    
    //-----------------------------------------------------------------------
    // Phase 1: Distance-based culling
    //-----------------------------------------------------------------------
    if (enableDistanceCulling && distanceToCamera > maxRenderDistance)
    {
        // Particle is too far away - mark as invisible and culled
        result.visible = 0;
        result.lodLevel = 3;  // LOD 3 = culled
        cullingResults[index] = result;
        return;
    }
    
    //-----------------------------------------------------------------------
    // Phase 2: Frustum culling with scaled particle radius
    //-----------------------------------------------------------------------
    
    // Apply global scale and clamp particle radius to reasonable bounds
    float scaledRadius = clamp(particle.radius * globalScale, 
                              minParticleRadius, maxParticleRadius);
    
    // Test if particle sphere intersects or is inside the view frustum
    result.visible = SphereInFrustum(particle.position, scaledRadius) ? 1 : 0;
    
    //-----------------------------------------------------------------------
    // Phase 3: LOD level calculation for visible particles
    //-----------------------------------------------------------------------
    
    // Calculate appropriate level of detail based on distance
    result.lodLevel = CalculateLODLevel(distanceToCamera);
    
    // If calculated LOD level is 3 (too far), mark as invisible
    if (result.lodLevel >= 3)
    {
        result.visible = 0;
    }
    
    // Write final culling results to output buffer
    cullingResults[index] = result;
}

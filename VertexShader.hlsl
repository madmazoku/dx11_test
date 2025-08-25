/**
 * @file VertexShader.hlsl
 * @brief Vertex shader for multi-type particle system rendering pipeline
 * 
 * This vertex shader serves as the first stage in the particle rendering pipeline.
 * It processes particle data from structured buffers and prepares it for the
 * geometry shader, which will generate icosphere geometry for each particle.
 * 
 * Key features:
 * - Processes particles directly from structured buffer (no traditional vertex buffer)
 * - Transforms particle positions to world space
 * - Passes through particle properties (type, radius) for geometry generation
 * - Supports global scaling for zoom and display adjustments
 * - Optimized for instanced particle rendering
 * 
 * The shader uses the vertex ID to index directly into the particle buffer,
 * allowing efficient processing of dynamic particle counts without the need
 * for traditional vertex buffer updates.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

/**
 * @struct Particle
 * @brief Input particle data structure from structured buffer
 * 
 * Contains all particle properties needed for rendering, including position,
 * physical properties, and type information for material selection.
 */
struct Particle
{
    float3 position;      ///< Current world position
    float3 oldPosition;   ///< Previous position (unused in vertex shader)
    float3 velocity;      ///< Current velocity (unused in vertex shader)
    float3 acceleration;  ///< Current acceleration (unused in vertex shader)
    
    uint typeId;          ///< Particle type for material/color selection
    float mass;           ///< Particle mass (unused in vertex shader)
    float radius;         ///< Particle radius for icosphere scaling
    float charge;         ///< Particle charge (unused in vertex shader)
    
    float temperature;    ///< Particle temperature (unused in vertex shader)
    float age;            ///< Particle age (unused in vertex shader)
    uint collisionCount;  ///< Collision count (unused in vertex shader)
    uint flags;           ///< State flags (unused in vertex shader)
};

/**
 * @struct VS_OUTPUT
 * @brief Vertex shader output structure for geometry shader input
 * 
 * Passes transformed particle data to the geometry shader for icosphere generation.
 */
struct VS_OUTPUT
{
    float4 position : POSITION;  ///< Transformed particle center position
    uint typeId : TEXCOORD0;     ///< Particle type ID for material selection
    float radius : TEXCOORD1;    ///< Scaled particle radius for geometry size
    float3 worldPos : TEXCOORD2; ///< World space position for lighting calculations
};

// Input particle data from structured buffer
StructuredBuffer<Particle> particles : register(t0);  ///< Particle data buffer (read-only)

/**
 * @brief Transform matrices and camera parameters constant buffer
 * 
 * Contains the transformation matrices and camera information needed
 * for world space transformations and scaling calculations.
 */
cbuffer TransformBuffer : register(b0)
{
    matrix viewProjectionMatrix; ///< Combined view * projection matrix
    matrix worldMatrix;          ///< World transformation matrix
    float3 cameraPos;            ///< Camera position in world space
    float globalScale;           ///< Global scaling factor for particles
};

/**
 * @brief Main vertex shader entry point
 * 
 * Processes a single particle by transforming its position to world space
 * and preparing data for the geometry shader. Uses vertex ID to index
 * directly into the particle structured buffer.
 * 
 * @param vertexID System-generated vertex ID used as particle index
 * @return VS_OUTPUT structure with transformed particle data
 */
VS_OUTPUT VSMain(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    // Fetch particle data using vertex ID as index
    Particle particle = particles[vertexID];
    
    // Transform particle position to world space
    float3 worldPos = mul(float4(particle.position, 1.0f), worldMatrix).xyz;
    
    //-----------------------------------------------------------------------
    // Prepare output data for geometry shader
    //-----------------------------------------------------------------------
    
    // Pass world position as center point for icosphere generation
    output.position = float4(worldPos, 1.0f);  // Will be transformed to clip space in geometry shader
    
    // Pass particle type for material/color selection in pixel shader
    output.typeId = particle.typeId;
    
    // Apply global scaling to particle radius
    output.radius = particle.radius * globalScale;
    
    // Store world position for lighting calculations in pixel shader
    output.worldPos = worldPos;
    
    return output;
}

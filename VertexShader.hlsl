// Vertex Shader for Multi-Type Particle System
// Prepares particle data for geometry shader icosphere generation

struct Particle
{
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

struct VS_OUTPUT
{
    float4 position : POSITION;
    uint typeId : TEXCOORD0;
    float radius : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

// Particle buffer
StructuredBuffer<Particle> particles : register(t0);

// Transform constants
cbuffer TransformBuffer : register(b0)
{
    matrix viewProjectionMatrix;
    matrix worldMatrix;
    float3 cameraPos;
    float globalScale;
};

VS_OUTPUT VSMain(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    // Get particle data
    Particle particle = particles[vertexID];
    
    // Transform world position
    float3 worldPos = mul(float4(particle.position, 1.0f), worldMatrix).xyz;
    
    // Pass data to geometry shader
    output.position = float4(worldPos, 1.0f);  // Will be transformed in GS
    output.typeId = particle.typeId;
    output.radius = particle.radius;
    output.worldPos = worldPos;
    
    return output;
}

// Geometry shader input structure
struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 velocity : VELOCITY;
    uint pointId : POINTID;
};

// Geometry shader output structure
struct GSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 velocity : VELOCITY;
    uint pointId : POINTID;
};

// Constant buffer for sphere generation
cbuffer SphereBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 worldMatrix;
    float3 cameraPos;
    float sphereRadius;
};

// Generate sphere vertices (icosphere approximation)
static const float3 sphereVertices[12] = {
    float3(-1, 1.618f, 0), float3(1, 1.618f, 0), float3(-1, -1.618f, 0), float3(1, -1.618f, 0),
    float3(0, -1, 1.618f), float3(0, 1, 1.618f), float3(0, -1, -1.618f), float3(0, 1, -1.618f),
    float3(1.618f, 0, -1), float3(1.618f, 0, 1), float3(-1.618f, 0, -1), float3(-1.618f, 0, 1)
};

static const uint3 sphereIndices[20] = {
    uint3(0, 11, 5), uint3(0, 5, 1), uint3(0, 1, 7), uint3(0, 7, 10), uint3(0, 10, 11),
    uint3(1, 5, 9), uint3(5, 11, 4), uint3(11, 10, 2), uint3(10, 7, 6), uint3(7, 1, 8),
    uint3(3, 9, 4), uint3(3, 4, 2), uint3(3, 2, 6), uint3(3, 6, 8), uint3(3, 8, 9),
    uint3(4, 9, 5), uint3(2, 4, 11), uint3(6, 2, 10), uint3(8, 6, 7), uint3(9, 8, 1)
};

// Geometry shader main function
[maxvertexcount(60)] // 20 triangles * 3 vertices
void GSMain(point VSOutput input[1], inout TriangleStream<GSOutput> triangleStream)
{
    float3 center = input[0].worldPos;
    float radius = sphereRadius;
    
    // Generate sphere triangles
    for (uint i = 0; i < 20; i++)
    {
        uint3 indices = sphereIndices[i];
        
        for (uint j = 0; j < 3; j++)
        {
            GSOutput output;
            
            float3 spherePos = normalize(sphereVertices[indices[j]]) * radius;
            float3 worldPos = center + spherePos;
            
            output.position = mul(float4(worldPos, 1.0f), viewProjectionMatrix);
            output.normal = normalize(spherePos);
            output.worldPos = worldPos;
            output.velocity = input[0].velocity;
            output.pointId = input[0].pointId;
            
            // Simple UV mapping based on normal
            output.texcoord = float2(
                atan2(output.normal.x, output.normal.z) / (2.0f * 3.14159f) + 0.5f,
                acos(output.normal.y) / 3.14159f
            );
            
            triangleStream.Append(output);
        }
        triangleStream.RestartStrip();
    }
}

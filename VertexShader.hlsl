struct Point
{
    float3 position;
    float3 oldPosition;
    float3 acceleration;
};

// Input buffer from the compute shader
StructuredBuffer<Point> pointsOut : register(t0);

// Constant buffer for transformation matrices
cbuffer TransformBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 worldMatrix;
    float3 cameraPos;
    float sphereRadius;
};

// Output structure for the vertex shader
struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 velocity : VELOCITY;
    uint pointId : POINTID;
};

// Vertex shader main function
VSOutput VSMain(uint id : SV_VertexID)
{
    VSOutput output;
    
    // Extract the position from pointsOut
    Point p = pointsOut[id];
    
    // Transform to world space then to clip space
    float4 worldPos = mul(float4(p.position, 1.0f), worldMatrix);
    output.position = mul(worldPos, viewProjectionMatrix);
    output.worldPos = worldPos.xyz;
    output.velocity = p.position - p.oldPosition; // Current velocity from Verlet
    output.pointId = id;
    
    return output;
}

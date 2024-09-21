struct Point
{
    float3 position;
    float3 velocity;
};

// Input buffer from the compute shader
StructuredBuffer<Point> pointsOut : register(t0);

// Output structure for the vertex shader
struct VSOutput
{
    float4 position : SV_POSITION;
};

// Vertex shader main function
VSOutput VSMain(uint id : SV_VertexID)
{
    VSOutput output;
    
    // Extract the position from pointsOut
    Point p = pointsOut[id];
//    output.position = float4(p.position, 1.0f); // Convert to float4 for SV_POSITION
    output.position = float4(float(id), 2.0f, 3.0f, 4.0f);
    
    return output;
}

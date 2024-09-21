// Geometry shader input structure
struct VSOutput
{
    float4 position : SV_POSITION;
};

// Geometry shader output structure
struct GSOutput
{
    float4 position : SV_POSITION;
};

// Geometry shader main function
[maxvertexcount(1)]
void GSMain(point VSOutput input[1], inout PointStream<GSOutput> pointStream)
{
    GSOutput output;
    output.position = input[0].position;
    pointStream.Append(output);
}

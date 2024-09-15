struct Point
{
    float3 position;
    float3 velocity;
};

StructuredBuffer<Point> pointsIn : register(t0);
RWStructuredBuffer<Point> pointsOut : register(u0);

static const float k = 0.01f;
static const float m = 1.0f;
static const float r0 = 0.2f;
static const float dt = 0.01f;

float calcForce(float r)
{
    return k * (r - r0);
}

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    uint index = dispatchID.x;
    Point p = pointsIn[index];

    uint numStructs;
    uint stride;

    pointsIn.GetDimensions(numStructs, stride);
    
    float3 totalForce = float3(0, 0, 0);
    for (uint i = 0; i < numStructs; i++)
    {
        if (i != index)
        {
            float3 d = pointsIn[i].position - p.position;
            float r = length(d);
            if (r > 0.0001f)
            {
                float3 nd = d / r;
                float forceValue = calcForce(r);
                totalForce += d * forceValue / r;
            }
        }
    }

    float3 totalAcceleration = totalForce / m;
    p.position += p.velocity * dt;
    p.velocity += totalAcceleration * dt;

    pointsOut[index] = p;
}

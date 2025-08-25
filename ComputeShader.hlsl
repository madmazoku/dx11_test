struct Point
{
    float3 position;
    float3 oldPosition;
    float3 acceleration;
};

StructuredBuffer<Point> pointsIn : register(t0);
RWStructuredBuffer<Point> pointsOut : register(u0);

static const float k = 0.5f;
static const float m = 1.0f;
static const float r0 = 0.3f;
static const float dt = 0.016f;  // ~60fps
static const float damping = 0.98f;
static const float3 gravity = float3(0.0f, -9.81f, 0.0f);

float3 calcSpringForce(float3 displacement, float distance)
{
    if (distance < 0.0001f) return float3(0, 0, 0);
    
    float3 direction = displacement / distance;
    float force = k * (distance - r0);
    return direction * force;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    uint index = dispatchID.x;
    
    uint numStructs;
    uint stride;
    pointsIn.GetDimensions(numStructs, stride);
    
    if (index >= numStructs) return;
    
    Point p = pointsIn[index];
    
    // Calculate forces from other particles
    float3 totalForce = gravity * m; // Add gravity
    
    for (uint i = 0; i < numStructs; i++)
    {
        if (i != index)
        {
            float3 displacement = pointsIn[i].position - p.position;
            float distance = length(displacement);
            
            if (distance > 0.0001f && distance < 2.0f) // Only interact with nearby particles
            {
                totalForce += calcSpringForce(displacement, distance);
            }
        }
    }
    
    // Verlet integration
    float3 newAcceleration = totalForce / m;
    float3 newPosition = 2.0f * p.position - p.oldPosition + newAcceleration * dt * dt;
    
    // Apply damping
    newPosition = p.position + (newPosition - p.position) * damping;
    
    // Boundary constraints (simple box)
    newPosition = clamp(newPosition, float3(-5.0f, -5.0f, -5.0f), float3(5.0f, 5.0f, 5.0f));
    
    // Update point
    Point newPoint;
    newPoint.oldPosition = p.position;
    newPoint.position = newPosition;
    newPoint.acceleration = newAcceleration;
    
    pointsOut[index] = newPoint;
}

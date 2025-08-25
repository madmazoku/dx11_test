// Advanced Geometry Shader for Icosphere Generation with LOD
// Generates icospheres (subdivided icosahedrons) for particle rendering

struct VS_OUTPUT
{
    float4 position : POSITION;
    uint typeId : TEXCOORD0;
    float radius : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 viewDir : TEXCOORD2;
    uint typeId : TEXCOORD3;
    float distanceToCamera : TEXCOORD4;
};

// Transform and camera constants
cbuffer TransformBuffer : register(b0)
{
    matrix viewProjectionMatrix;
    matrix worldMatrix;
    float3 cameraPos;
    float globalScale;
};

// LOD and culling constants
cbuffer CullingBuffer : register(b1)
{
    float lodDistance0;     // High detail distance
    float lodDistance1;     // Medium detail distance  
    float lodDistance2;     // Low detail distance
    float maxRenderDistance;
    bool enableLOD;
    bool enableDistanceCulling;
    float2 padding;
};

// Base icosahedron vertices (12 vertices)
static const float PHI = 1.618033988749895f; // Golden ratio
static const float INV_SQRT_5 = 0.4472135954999579f;

static const float3 IcosahedronVertices[12] = 
{
    float3(-1,  PHI,  0) * INV_SQRT_5,
    float3( 1,  PHI,  0) * INV_SQRT_5,
    float3(-1, -PHI,  0) * INV_SQRT_5,
    float3( 1, -PHI,  0) * INV_SQRT_5,
    float3( 0, -1,  PHI) * INV_SQRT_5,
    float3( 0,  1,  PHI) * INV_SQRT_5,
    float3( 0, -1, -PHI) * INV_SQRT_5,
    float3( 0,  1, -PHI) * INV_SQRT_5,
    float3( PHI,  0, -1) * INV_SQRT_5,
    float3( PHI,  0,  1) * INV_SQRT_5,
    float3(-PHI,  0, -1) * INV_SQRT_5,
    float3(-PHI,  0,  1) * INV_SQRT_5
};

// Base icosahedron faces (20 triangles)
static const uint3 IcosahedronFaces[20] = 
{
    uint3(0, 11, 5),   uint3(0, 5, 1),    uint3(0, 1, 7),    uint3(0, 7, 10),   uint3(0, 10, 11),
    uint3(1, 5, 9),    uint3(5, 11, 4),   uint3(11, 10, 2),  uint3(10, 7, 6),   uint3(7, 1, 8),
    uint3(3, 9, 4),    uint3(3, 4, 2),    uint3(3, 2, 6),    uint3(3, 6, 8),    uint3(3, 8, 9),
    uint3(4, 9, 5),    uint3(2, 4, 11),   uint3(6, 2, 10),   uint3(8, 6, 7),    uint3(9, 8, 1)
};

// Determine LOD level based on distance
uint GetLODLevel(float distanceToCamera)
{
    if (!enableLOD) return 2;  // High detail if LOD disabled
    
    if (distanceToCamera <= lodDistance0) return 2;      // High detail (2 subdivisions)
    else if (distanceToCamera <= lodDistance1) return 1; // Medium detail (1 subdivision)  
    else if (distanceToCamera <= lodDistance2) return 0; // Low detail (base icosahedron)
    else return 0;  // Very low detail
}

// Get midpoint on unit sphere
float3 GetMidpointOnSphere(float3 v1, float3 v2)
{
    float3 mid = (v1 + v2) * 0.5f;
    return normalize(mid);
}

// Generate triangle with given vertices
void EmitTriangle(float3 center, float radius, float3 v0, float3 v1, float3 v2, 
                 uint typeId, float distanceToCamera, inout TriangleStream<GS_OUTPUT> triStream)
{
    GS_OUTPUT output;
    output.typeId = typeId;
    output.distanceToCamera = distanceToCamera;
    
    // Scale and position vertices
    float3 worldV0 = center + v0 * radius * globalScale;
    float3 worldV1 = center + v1 * radius * globalScale;
    float3 worldV2 = center + v2 * radius * globalScale;
    
    // Calculate face normal (for flat shading, if needed)
    float3 edge1 = worldV1 - worldV0;
    float3 edge2 = worldV2 - worldV0;
    float3 faceNormal = normalize(cross(edge1, edge2));
    
    // Emit vertex 0
    output.worldPos = worldV0;
    output.normal = normalize(v0);  // Smooth normals (vertex normals point outward from center)
    output.viewDir = normalize(cameraPos - worldV0);
    output.position = mul(float4(worldV0, 1.0f), viewProjectionMatrix);
    triStream.Append(output);
    
    // Emit vertex 1
    output.worldPos = worldV1;
    output.normal = normalize(v1);
    output.viewDir = normalize(cameraPos - worldV1);
    output.position = mul(float4(worldV1, 1.0f), viewProjectionMatrix);
    triStream.Append(output);
    
    // Emit vertex 2
    output.worldPos = worldV2;
    output.normal = normalize(v2);
    output.viewDir = normalize(cameraPos - worldV2);
    output.position = mul(float4(worldV2, 1.0f), viewProjectionMatrix);
    triStream.Append(output);
    
    triStream.RestartStrip();
}

// Subdivide triangle recursively
void SubdivideTriangle(float3 center, float radius, float3 v0, float3 v1, float3 v2, 
                      uint subdivisionLevel, uint typeId, float distanceToCamera, 
                      inout TriangleStream<GS_OUTPUT> triStream)
{
    if (subdivisionLevel == 0)
    {
        EmitTriangle(center, radius, v0, v1, v2, typeId, distanceToCamera, triStream);
        return;
    }
    
    // Calculate midpoints on sphere surface
    float3 m01 = GetMidpointOnSphere(v0, v1);
    float3 m12 = GetMidpointOnSphere(v1, v2);
    float3 m20 = GetMidpointOnSphere(v2, v0);
    
    // Recursively subdivide into 4 triangles
    SubdivideTriangle(center, radius, v0, m01, m20, subdivisionLevel - 1, typeId, distanceToCamera, triStream);
    SubdivideTriangle(center, radius, v1, m12, m01, subdivisionLevel - 1, typeId, distanceToCamera, triStream);
    SubdivideTriangle(center, radius, v2, m20, m12, subdivisionLevel - 1, typeId, distanceToCamera, triStream);
    SubdivideTriangle(center, radius, m01, m12, m20, subdivisionLevel - 1, typeId, distanceToCamera, triStream);
}

[maxvertexcount(320)]  // Max vertices for 2-level subdivision icosahedron
void GSMain(point VS_OUTPUT input[1], inout TriangleStream<GS_OUTPUT> triStream)
{
    float3 center = input[0].worldPos;
    float radius = input[0].radius;
    uint typeId = input[0].typeId;
    
    // Calculate distance to camera
    float distanceToCamera = length(cameraPos - center);
    
    // Distance culling
    if (enableDistanceCulling && distanceToCamera > maxRenderDistance)
        return;
    
    // Determine LOD level
    uint lodLevel = GetLODLevel(distanceToCamera);
    
    // Generate icosphere by subdividing base icosahedron faces
    for (uint faceIdx = 0; faceIdx < 20; faceIdx++)
    {
        uint3 face = IcosahedronFaces[faceIdx];
        float3 v0 = IcosahedronVertices[face.x];
        float3 v1 = IcosahedronVertices[face.y];
        float3 v2 = IcosahedronVertices[face.z];
        
        SubdivideTriangle(center, radius, v0, v1, v2, lodLevel, typeId, distanceToCamera, triStream);
    }
}

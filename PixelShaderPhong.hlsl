// Pixel Shader for Advanced Phong Shading with Material Properties
// Supports metallic/dielectric materials for multi-type particles

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 viewDir : TEXCOORD2;
    uint typeId : TEXCOORD3;
    float distanceToCamera : TEXCOORD4;
};

// Material properties for each particle type
struct MaterialProperties
{
    float3 diffuseColor;
    float metallic;
    float3 specularColor;
    float roughness;
    float shininess;
    float reflectance;
    float emissive;
    float padding;
    float3 emissiveColor;
    float padding2;
};

// Lighting constants
cbuffer LightBuffer : register(b0)
{
    float3 lightDirection;
    float lightIntensity;
    float3 lightColor;
    float ambientIntensity;
    float3 cameraPos;
    float specularPower;
};

// Material buffer (array of materials for each particle type)
StructuredBuffer<MaterialProperties> materials : register(t0);

// Fresnel reflection calculation for metals/dielectrics
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// Distribution function for specular highlights
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265f * denom * denom;
    
    return num / denom;
}

// Geometry function for occlusion
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float4 PSMain(GS_OUTPUT input) : SV_TARGET
{
    // Get material properties for this particle type
    MaterialProperties material = materials[input.typeId];
    
    // Normalize vectors
    float3 N = normalize(input.normal);
    float3 V = normalize(input.viewDir);
    float3 L = normalize(-lightDirection);
    float3 H = normalize(V + L);
    
    // Calculate basic lighting factors
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    
    // Base color and material properties
    float3 albedo = material.diffuseColor;
    float metallic = material.metallic;
    float roughness = max(material.roughness, 0.04f); // Prevent division by zero
    
    // Calculate F0 (base reflectance)
    float3 F0 = lerp(float3(material.reflectance, material.reflectance, material.reflectance), 
                     albedo, metallic);
    
    // Ambient lighting
    float3 ambient = ambientIntensity * lightColor * albedo;
    
    // If in shadow or no light contribution, return ambient + emissive
    if (NdotL <= 0.0f)
    {
        float3 emissive = material.emissive * material.emissiveColor;
        return float4(ambient + emissive, 1.0f);
    }
    
    // Cook-Torrance BRDF
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    
    // Calculate specular and diffuse contributions
    float3 numerator = D * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.001f; // Prevent division by zero
    float3 specular = numerator / denominator;
    
    // Energy conservation - what isn't specular is diffuse
    float3 kS = F;  // Specular contribution
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;  // Diffuse contribution
    kD *= 1.0f - metallic;  // Metals don't have diffuse lighting
    
    // Lambert diffuse
    float3 diffuse = kD * albedo / 3.14159265f;
    
    // Combine lighting
    float3 radiance = lightColor * lightIntensity;
    float3 Lo = (diffuse + specular) * radiance * NdotL;
    
    // Add emissive contribution
    float3 emissive = material.emissive * material.emissiveColor;
    
    // Final color
    float3 color = ambient + Lo + emissive;
    
    // Simple tone mapping and gamma correction
    color = color / (color + float3(1.0f, 1.0f, 1.0f));  // Reinhard tone mapping
    color = pow(color, float3(1.0f/2.2f, 1.0f/2.2f, 1.0f/2.2f));  // Gamma correction
    
    // Distance-based alpha (optional fading)
    float alpha = 1.0f;
    if (input.distanceToCamera > 50.0f)
    {
        alpha = 1.0f - saturate((input.distanceToCamera - 50.0f) / 50.0f);
    }
    
    return float4(color, alpha);
}

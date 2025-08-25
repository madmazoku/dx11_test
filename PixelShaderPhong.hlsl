/**
 * @file PixelShaderPhong.hlsl
 * @brief Advanced Physically-Based Rendering (PBR) pixel shader with Cook-Torrance BRDF
 * 
 * This pixel shader implements a sophisticated physically-based rendering model using
 * the Cook-Torrance bidirectional reflectance distribution function (BRDF). It supports
 * both metallic and dielectric materials with realistic light interaction, including
 * proper energy conservation, Fresnel reflectance, and microfacet distribution.
 * 
 * Key features:
 * - Cook-Torrance BRDF for physically accurate lighting
 * - Metallic/dielectric workflow for material variety
 * - Fresnel reflectance using Schlick's approximation
 * - GGX (Trowbridge-Reitz) microfacet distribution
 * - Smith geometry function for masking/shadowing
 * - Energy conservation between specular and diffuse
 * - Emissive material support for glowing particles
 * - Distance-based alpha fading for depth effects
 * - Tone mapping and gamma correction for proper display
 * 
 * The shader processes geometry data from the icosphere geometry shader and applies
 * realistic lighting calculations to create visually convincing 3D particle effects.
 * Each particle type can have unique material properties stored in a structured buffer.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

/**
 * @struct GS_OUTPUT
 * @brief Input structure from geometry shader containing vertex and lighting data
 * 
 * Receives all necessary information from the geometry shader to perform
 * physically-based lighting calculations on each pixel.
 */
struct GS_OUTPUT
{
    float4 position : SV_POSITION;      ///< Screen space position for rasterization
    float3 worldPos : TEXCOORD0;        ///< World space position for lighting calculations
    float3 normal : TEXCOORD1;          ///< Surface normal vector (normalized)
    float3 viewDir : TEXCOORD2;         ///< View direction vector from surface to camera
    uint typeId : TEXCOORD3;            ///< Particle type ID for material lookup
    float distanceToCamera : TEXCOORD4; ///< Distance to camera for effects and culling
};

/**
 * @struct MaterialProperties
 * @brief Comprehensive material definition for physically-based rendering
 * 
 * Defines all material properties needed for realistic PBR lighting calculations.
 * Each particle type can have unique material characteristics, allowing for
 * diverse visual effects like metals, ceramics, plastics, and emissive materials.
 */
struct MaterialProperties
{
    float3 diffuseColor;    ///< Base albedo color (RGB) - what the material looks like in diffuse lighting
    float metallic;         ///< Metallic factor [0=dielectric, 1=metal] - controls conductivity
    float3 specularColor;   ///< Specular tint color (usually white for dielectrics, colored for metals)
    float roughness;        ///< Surface roughness [0=mirror, 1=completely rough] - controls highlight sharpness
    float shininess;        ///< Legacy Phong shininess parameter (for backward compatibility)
    float reflectance;      ///< Base reflectance at normal incidence for dielectrics (F0)
    float emissive;         ///< Emissive intensity factor - how much the material glows
    float padding;          ///< Padding for 16-byte alignment
    float3 emissiveColor;   ///< Color of emissive glow (RGB)
    float padding2;         ///< Additional padding for alignment
};

//=============================================================================
// Constant Buffers and Resources
//=============================================================================

/**
 * @brief Global lighting parameters constant buffer
 * 
 * Contains all lighting-related parameters that remain constant across
 * all pixels in a frame, including light direction, intensity, and camera position.
 */
cbuffer LightBuffer : register(b0)
{
    float3 lightDirection;  ///< Primary directional light direction (normalized)
    float lightIntensity;   ///< Light intensity multiplier for brightness control
    float3 lightColor;      ///< Light color (RGB) - usually white but can be tinted
    float ambientIntensity; ///< Global ambient light intensity [0-1]
    float3 cameraPos;       ///< Camera position in world space for view calculations
    float specularPower;    ///< Legacy specular power (for backward compatibility)
};

/**
 * @brief Material properties lookup table
 * 
 * Structured buffer containing material definitions for each particle type.
 * Indexed by the typeId from the geometry shader to retrieve appropriate
 * material properties for PBR calculations.
 */
StructuredBuffer<MaterialProperties> materials : register(t0);

//=============================================================================
// Physically-Based Rendering Functions
//=============================================================================

/**
 * @brief Fresnel reflectance using Schlick's approximation
 * @param cosTheta Cosine of angle between view direction and half-vector
 * @param F0 Base reflectance at normal incidence
 * @return Fresnel reflectance value for the given angle
 * 
 * Calculates the Fresnel effect, which determines how much light is reflected
 * vs transmitted based on viewing angle. More reflection at grazing angles.
 */
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

/**
 * @brief GGX/Trowbridge-Reitz normal distribution function
 * @param N Surface normal vector (normalized)
 * @param H Half-vector between light and view directions (normalized)
 * @param roughness Material roughness parameter [0-1]
 * @return Distribution term for specular BRDF
 * 
 * Describes the statistical distribution of microfacet orientations.
 * Controls the shape and size of specular highlights.
 */
/**
 * @brief GGX/Trowbridge-Reitz normal distribution function
 * @param N Surface normal vector (normalized)
 * @param H Half-vector between light and view directions (normalized)
 * @param roughness Material roughness parameter [0-1]
 * @return Distribution term for specular BRDF
 * 
 * Describes the statistical distribution of microfacet orientations.
 * Controls the shape and size of specular highlights.
 */
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;        // Square roughness for more perceptually linear response
    float a2 = a * a;                       // Roughness^4 for the distribution calculation
    float NdotH = max(dot(N, H), 0.0f);     // Cosine of angle between normal and half-vector
    float NdotH2 = NdotH * NdotH;           // Squared for efficiency
    
    float num = a2;                         // Numerator of GGX distribution
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265f * denom * denom;    // π * (denominator)^2
    
    return num / denom;                     // Final GGX distribution value
}

/**
 * @brief Smith geometry function - single direction masking/shadowing
 * @param NdotV Cosine of angle between normal and view/light direction
 * @param roughness Material roughness parameter [0-1]
 * @return Geometry masking factor for one direction
 * 
 * Describes the probability that microfacets are visible (not shadowed)
 * by other microfacets when viewed from a particular direction.
 */
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);          // Remapping for direct lighting
    float k = (r * r) / 8.0f;              // Schlick-GGX parameter
    
    float num = NdotV;                      // Cosine term
    float denom = NdotV * (1.0f - k) + k;  // Schlick approximation denominator
    
    return num / denom;                     // Geometry factor for this direction
}

/**
 * @brief Complete Smith geometry function for masking and shadowing
 * @param N Surface normal vector (normalized)
 * @param V View direction vector (normalized)
 * @param L Light direction vector (normalized)
 * @param roughness Material roughness parameter [0-1]
 * @return Combined geometry factor accounting for both masking and shadowing
 * 
 * Combines masking (view direction) and shadowing (light direction) effects.
 * Describes how much of the microfacet surface is visible to both camera and light.
 */
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);     // View angle factor
    float NdotL = max(dot(N, L), 0.0f);     // Light angle factor
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);  // Masking (view)
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);  // Shadowing (light)
    
    return ggx1 * ggx2;                     // Combined masking/shadowing
}

//=============================================================================
// Main Pixel Shader
//=============================================================================

/**
 * @brief Main pixel shader entry point - Physically-Based Rendering with Cook-Torrance BRDF
 * @param input Interpolated vertex data from geometry shader
 * @return Final pixel color with alpha
 * 
 * Implements a complete PBR lighting model using the Cook-Torrance BRDF.
 * Supports both metallic and dielectric materials with proper energy conservation,
 * includes emissive materials, and applies tone mapping and gamma correction.
 * 
 * The lighting calculation follows this structure:
 * 1. Material property lookup based on particle type
 * 2. Vector calculations (normal, view, light, half-vector)
 * 3. Cook-Torrance BRDF evaluation (D*G*F terms)
 * 4. Energy conservation between specular and diffuse
 * 5. Final color composition with ambient and emissive
 * 6. Post-processing (tone mapping, gamma correction, distance fade)
 */
float4 PSMain(GS_OUTPUT input) : SV_TARGET
{
    // =========================================================================
    // Material Setup and Vector Calculations
    // =========================================================================
    
    // Retrieve material properties for this particle type
    MaterialProperties material = materials[input.typeId];
    
    // Normalize all input vectors for accurate lighting calculations
    float3 N = normalize(input.normal);         // Surface normal
    float3 V = normalize(input.viewDir);        // View direction (surface to camera)
    float3 L = normalize(-lightDirection);      // Light direction (toward light)
    float3 H = normalize(V + L);                // Half-vector between view and light
    
    // Calculate fundamental dot products for lighting
    float NdotL = max(dot(N, L), 0.0f);        // Lambert factor (surface facing light)
    float NdotV = max(dot(N, V), 0.0f);        // View factor (surface facing camera)
    
    // =========================================================================
    // Material Property Setup
    // =========================================================================
    
    float3 albedo = material.diffuseColor;                      // Base surface color
    float metallic = material.metallic;                         // Metallic workflow factor
    float roughness = max(material.roughness, 0.04f);          // Prevent division by zero
    
    // Calculate base reflectance (F0) based on metallic workflow
    // Dielectrics: use reflectance value (typically 0.04 for common materials)
    // Metals: use albedo color as F0 (metals have colored reflectance)
    float3 F0 = lerp(float3(material.reflectance, material.reflectance, material.reflectance), 
                     albedo, metallic);
    
    // =========================================================================
    // Ambient Lighting (Base Illumination)
    // =========================================================================
    
    float3 ambient = ambientIntensity * lightColor * albedo;
    
    // Early exit for surfaces not facing the light (back-facing or in shadow)
    if (NdotL <= 0.0f)
    {
        float3 emissive = material.emissive * material.emissiveColor;
        return float4(ambient + emissive, 1.0f);
    }
    
    // =========================================================================
    // Cook-Torrance BRDF Calculation
    // =========================================================================
    
    // The Cook-Torrance BRDF consists of three main terms:
    // D: Normal Distribution Function (how microfacets are oriented)
    // G: Geometry Function (masking/shadowing of microfacets)  
    // F: Fresnel Function (reflection vs refraction ratio)
    float D = DistributionGGX(N, H, roughness);                    // Microfacet distribution
    float G = GeometrySmith(N, V, L, roughness);                   // Geometric attenuation
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);          // Fresnel reflectance
    
    // Cook-Torrance specular BRDF: (D * G * F) / (4 * NdotV * NdotL)
    float3 numerator = D * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.001f;            // Small epsilon prevents division by zero
    float3 specular = numerator / denominator;
    
    // =========================================================================
    // Energy Conservation and Diffuse Calculation
    // =========================================================================
    
    // Energy conservation: light is either reflected (specular) or refracted (diffuse)
    float3 kS = F;                              // Specular reflection ratio
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS; // Diffuse refraction ratio
    kD *= 1.0f - metallic;                      // Metals don't have diffuse reflection
    
    // Lambertian diffuse BRDF: albedo / π
    float3 diffuse = kD * albedo / 3.14159265f;
    
    // =========================================================================
    // Final Lighting Composition
    // =========================================================================
    
    // Combine diffuse and specular with light color and intensity
    float3 radiance = lightColor * lightIntensity;
    float3 Lo = (diffuse + specular) * radiance * NdotL;         // Outgoing light
    
    // Add emissive contribution (self-illumination)
    float3 emissive = material.emissive * material.emissiveColor;
    
    // Final color before post-processing
    float3 color = ambient + Lo + emissive;
    
    // =========================================================================
    // Post-Processing: Tone Mapping and Gamma Correction
    // =========================================================================
    
    // Reinhard tone mapping: color / (color + 1) - maps HDR to [0,1] range
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    
    // Gamma correction: linear to sRGB conversion (γ = 2.2)
    color = pow(color, float3(1.0f/2.2f, 1.0f/2.2f, 1.0f/2.2f));
    
    // =========================================================================
    // Distance-Based Alpha Fading
    // =========================================================================
    
    // Optional distance-based transparency for atmospheric perspective
    float alpha = 1.0f;
    if (input.distanceToCamera > 50.0f)
    {
        alpha = 1.0f - saturate((input.distanceToCamera - 50.0f) / 50.0f);
    }
    
    return float4(color, alpha);
}

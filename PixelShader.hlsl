// Pixel shader input structure (from geometry shader)
struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 velocity : VELOCITY;
    uint pointId : POINTID;
};

// Constant buffer for lighting
cbuffer LightBuffer : register(b0)
{
    float3 lightDirection;
    float lightIntensity;
    float3 lightColor;
    float ambientIntensity;
    float3 cameraPos;
    float specularPower;
};

// Pixel shader main function
float4 PSMain(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-lightDirection);
    
    // Diffuse lighting
    float diffuse = max(dot(normal, lightDir), 0.0f);
    
    // Specular lighting
    float3 viewDir = normalize(cameraPos - input.worldPos);
    float3 reflectDir = reflect(-lightDir, normal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0f), specularPower);
    
    // Color based on velocity (for visual feedback)
    float speed = length(input.velocity);
    float3 velocityColor = lerp(float3(0.2f, 0.4f, 1.0f), float3(1.0f, 0.4f, 0.2f), saturate(speed * 10.0f));
    
    // Final color calculation
    float3 ambient = velocityColor * ambientIntensity;
    float3 diffuseColor = velocityColor * lightColor * diffuse * lightIntensity;
    float3 specularColor = lightColor * specular * lightIntensity;
    
    float3 finalColor = ambient + diffuseColor + specularColor;
    
    return float4(finalColor, 1.0f);
}

#version 330

// Input from vertex shader
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragTangent;
in vec3 fragBitangent;

// Output fragment color
out vec4 finalColor;

// Material textures
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;

// Material properties (fallback values)
uniform vec4 colDiffuse;
uniform float metallicValue;
uniform float roughnessValue;

// Texture availability flags
uniform bool hasAlbedoMap;
uniform bool hasNormalMap;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;
uniform bool hasAOMap;
uniform bool hasEmissiveMap;

// Lighting uniforms
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 viewPos;

// IBL uniforms
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// Feature toggles
uniform bool enablePBR;
uniform bool enableNormalMapping;
uniform bool enableIBL;

const float PI = 3.14159265359;
const int MAX_REFLECTION_LOD = 4;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001);
}

// Geometry function (Smith's method with Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel equation (Schlick approximation)
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Fresnel with roughness for IBL
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Get normal from normal map or use vertex normal
vec3 GetNormal()
{
    if (enableNormalMapping && hasNormalMap)
    {
        // Sample normal from normal map
        vec3 tangentNormal = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;
        
        // Transform from tangent space to world space
        vec3 N = normalize(fragNormal);
        vec3 T = normalize(fragTangent);
        vec3 B = normalize(fragBitangent);
        mat3 TBN = mat3(T, B, N);
        
        return normalize(TBN * tangentNormal);
    }
    else
    {
        return normalize(fragNormal);
    }
}

void main()
{
    // Get material properties
    vec3 albedo = hasAlbedoMap ? texture(albedoMap, fragTexCoord).rgb : colDiffuse.rgb;
    float metallic = hasMetallicMap ? texture(metallicMap, fragTexCoord).r : metallicValue;
    float roughness = hasRoughnessMap ? texture(roughnessMap, fragTexCoord).r : roughnessValue;
    float ao = hasAOMap ? texture(aoMap, fragTexCoord).r : 1.0;
    vec3 emissive = hasEmissiveMap ? texture(emissiveMap, fragTexCoord).rgb : vec3(0.0);
    
    // Get normal
    vec3 N = GetNormal();
    vec3 V = normalize(viewPos - fragPosition);
    
    if (enablePBR)
    {
        // PBR Lighting calculation
        
        // Calculate reflectance at normal incidence
        // For dielectrics (non-metals), use 0.04
        // For metals, use the albedo color
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, albedo, metallic);
        
        // Directional light calculation
        vec3 L = normalize(-lightDirection);
        vec3 H = normalize(V + L);
        vec3 radiance = lightColor * lightIntensity;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // Calculate specular and diffuse components
        vec3 kS = F;  // Specular reflection
        vec3 kD = vec3(1.0) - kS;  // Diffuse reflection
        kD *= 1.0 - metallic;  // Metallic surfaces have no diffuse
        
        float NdotL = max(dot(N, L), 0.0);
        
        // Specular BRDF
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL;
        vec3 specular = numerator / max(denominator, 0.001);
        
        // Diffuse BRDF (Lambertian)
        vec3 diffuse = kD * albedo / PI;
        
        // Final outgoing radiance
        vec3 Lo = (diffuse + specular) * radiance * NdotL;
        
        // Ambient lighting with IBL
        vec3 ambient;
        if (enableIBL)
        {
            // IBL ambient lighting
            vec3 R = reflect(-V, N);
            
            // Fresnel for IBL
            vec3 F_ibl = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
            vec3 kS_ibl = F_ibl;
            vec3 kD_ibl = vec3(1.0) - kS_ibl;
            kD_ibl *= 1.0 - metallic;
            
            // Diffuse IBL
            vec3 irradiance = texture(irradianceMap, N).rgb;
            vec3 diffuse_ibl = irradiance * albedo;
            
            // Specular IBL
            vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * float(MAX_REFLECTION_LOD)).rgb;
            vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
            vec3 specular_ibl = prefilteredColor * (F_ibl * envBRDF.x + envBRDF.y);
            
            ambient = (kD_ibl * diffuse_ibl + specular_ibl) * ao;
        }
        else
        {
            // Simple ambient approximation
            ambient = vec3(0.03) * albedo * ao;
        }
        
        // Add emissive
        vec3 color = ambient + Lo + emissive;
        
        // HDR tonemapping (Reinhard)
        color = color / (color + vec3(1.0));
        
        // Gamma correction
        color = pow(color, vec3(1.0/2.2));
        
        finalColor = vec4(color, 1.0);
    }
    else
    {
        // Simple Blinn-Phong shading (fallback)
        vec3 L = normalize(-lightDirection);
        vec3 H = normalize(V + L);
        
        // Ambient
        vec3 ambient = vec3(0.1) * albedo;
        
        // Diffuse
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * albedo * lightColor * lightIntensity;
        
        // Specular (Blinn-Phong)
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        vec3 specular = spec * lightColor * lightIntensity * (1.0 - roughness);
        
        vec3 color = ambient + diffuse + specular + emissive;
        color *= ao;
        
        // Gamma correction
        color = pow(color, vec3(1.0/2.2));
        
        finalColor = vec4(color, 1.0);
    }
}

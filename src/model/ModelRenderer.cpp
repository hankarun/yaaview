#include "ModelRenderer.h"
#include "rlgl.h"
#include "raymath.h"
#include <iostream>

namespace AAV {

// Embedded PBR Vertex Shader
static const char* pbrVertexShader = R"(
#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec4 vertexTangent;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragTangent;
out vec3 fragBitangent;

void main()
{
    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    
    // Calculate TBN matrix for normal mapping
    vec3 T = normalize(vec3(matNormal * vec4(vertexTangent.xyz, 0.0)));
    vec3 N = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    
    // Re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    
    // Calculate bitangent using the tangent w component (handedness)
    vec3 B = cross(N, T) * vertexTangent.w;
    
    fragNormal = N;
    fragTangent = T;
    fragBitangent = B;
    
    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

// Embedded PBR Fragment Shader
static const char* pbrFragmentShader = R"(
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
)";


ModelRenderer::ModelRenderer()
    : wireframeMode(false)
    , showBoundingBox(false)
    , lightEnabled(true)
    , lightDirection({-0.5f, -1.0f, -0.3f})
    , lightColor(WHITE)
    , lightIntensity(1.0f)
    , shadersLoaded(false)
    , pbrEnabled(true)
    , normalMappingEnabled(true)
{
}

ModelRenderer::~ModelRenderer() {
    if (shadersLoaded) {
        UnloadShader(pbrShader);
    }
}

void ModelRenderer::Initialize() {
    // Load PBR shader from embedded strings
    pbrShader = LoadShaderFromMemory(pbrVertexShader, pbrFragmentShader);
    
    if (pbrShader.id == 0) {
        std::cerr << "Failed to load PBR shader, using default rendering" << std::endl;
        shadersLoaded = false;
    } else {
        std::cout << "PBR shader loaded successfully" << std::endl;
        shadersLoaded = true;
        
        // Get shader uniform locations (they're cached for performance)
        pbrShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(pbrShader, "mvp");
        pbrShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(pbrShader, "matModel");
        pbrShader.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(pbrShader, "matNormal");
        pbrShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(pbrShader, "viewPos");
        
        // Set texture sampler locations ONCE (tell shader which texture unit each sampler uses)
        int texUnit0 = 0;
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "albedoMap"), &texUnit0, SHADER_UNIFORM_INT);
        
        int texUnit1 = 1;
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "normalMap"), &texUnit1, SHADER_UNIFORM_INT);
        
        int texUnit2 = 2;
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "metallicMap"), &texUnit2, SHADER_UNIFORM_INT);
        
        int texUnit3 = 3;
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "roughnessMap"), &texUnit3, SHADER_UNIFORM_INT);
        
        int texUnit4 = 4;
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "aoMap"), &texUnit4, SHADER_UNIFORM_INT);
        
        int texUnit5 = 5;
        SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "emissiveMap"), &texUnit5, SHADER_UNIFORM_INT);
        
        std::cout << "Texture sampler locations configured" << std::endl;
    }
}

void ModelRenderer::Render(const Model& model, Camera3D camera) {
    if (!model.IsLoaded()) return;
    
    Matrix modelTransform = model.GetTransformMatrix();
    
    // Check if we have a node hierarchy
    auto rootNode = model.GetRootNode();
    if (rootNode) {
        // Render using hierarchical transforms
        RenderNodeHierarchy(rootNode.get(), modelTransform, model, camera);
    } else {
        // Fallback: render all meshes directly (flat hierarchy)
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(modelTransform));
        
        if (wireframeMode) {
            rlEnableWireMode();
        }
        
        const auto& meshes = model.GetMeshes();
        
        // Begin shader mode if PBR is enabled
        if (pbrEnabled && shadersLoaded && lightEnabled) {
            BeginShaderMode(pbrShader);
            
            // Set shader uniforms AFTER shader is active
            // Set lighting uniforms
            Vector3 normLightDir = Vector3Normalize(lightDirection);
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "lightDirection"), &normLightDir, SHADER_UNIFORM_VEC3);
            
            Vector3 lightColorVec = {lightColor.r / 255.0f, lightColor.g / 255.0f, lightColor.b / 255.0f};
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "lightColor"), &lightColorVec, SHADER_UNIFORM_VEC3);
            
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "lightIntensity"), &lightIntensity, SHADER_UNIFORM_FLOAT);
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "viewPos"), &camera.position, SHADER_UNIFORM_VEC3);
            
            // Set feature toggles
            int pbrEnabledInt = 1;
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "enablePBR"), &pbrEnabledInt, SHADER_UNIFORM_INT);
            
            int normalMappingEnabledInt = normalMappingEnabled ? 1 : 0;
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "enableNormalMapping"), &normalMappingEnabledInt, SHADER_UNIFORM_INT);
                        
        }
        
        for (const auto& meshData : meshes) {
            // Set material uniforms if PBR is enabled
            if (pbrEnabled && shadersLoaded && lightEnabled) {
                // Set material uniforms using the material index
                if (meshData.materialIndex >= 0 && meshData.materialIndex < (int)model.GetMaterials().size()) {
                    SetMaterialUniforms(model.GetMaterials()[meshData.materialIndex]);
                }
            }
            
            DrawMesh(meshData.mesh, meshData.material, MatrixIdentity());
            
            // Draw per-mesh bounding box if enabled
            if (showBoundingBox) {
                RenderMeshBoundingBox(meshData, modelTransform);
            }
        }
        
        // End shader mode if PBR is enabled
        if (pbrEnabled && shadersLoaded && lightEnabled) {
            EndShaderMode();
        }
        
        if (wireframeMode) {
            rlDisableWireMode();
        }
        
        rlPopMatrix();
    }
}

void ModelRenderer::RenderNodeHierarchy(NodeData* node, const Matrix& parentTransform, const Model& model, Camera3D camera) {
    if (!node) return;
    
    // Accumulate transform: local transform * parent transform
    Matrix worldTransform = MatrixMultiply(node->transform, parentTransform);
    
    // Render all meshes attached to this node
    if (!node->meshIndices.empty()) {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(worldTransform));
        
        if (wireframeMode) {
            rlEnableWireMode();
        }
        
        // Begin shader mode if PBR is enabled
        if (pbrEnabled && shadersLoaded && lightEnabled) {
            BeginShaderMode(pbrShader);
            
            // Set shader uniforms AFTER shader is active
            // Set lighting uniforms
            Vector3 normLightDir = Vector3Normalize(lightDirection);
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "lightDirection"), &normLightDir, SHADER_UNIFORM_VEC3);
            
            Vector3 lightColorVec = {lightColor.r / 255.0f, lightColor.g / 255.0f, lightColor.b / 255.0f};
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "lightColor"), &lightColorVec, SHADER_UNIFORM_VEC3);
            
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "lightIntensity"), &lightIntensity, SHADER_UNIFORM_FLOAT);
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "viewPos"), &camera.position, SHADER_UNIFORM_VEC3);
            
            // Set feature toggles
            int pbrEnabledInt = 1;
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "enablePBR"), &pbrEnabledInt, SHADER_UNIFORM_INT);
            
            int normalMappingEnabledInt = normalMappingEnabled ? 1 : 0;
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "enableNormalMapping"), &normalMappingEnabledInt, SHADER_UNIFORM_INT);
                    
        }
        
        const auto& meshes = model.GetMeshes();
        for (int meshIndex : node->meshIndices) {
            if (meshIndex >= 0 && meshIndex < (int)meshes.size()) {
                const auto& meshData = meshes[meshIndex];
                
                // Set material uniforms if PBR is enabled
                if (pbrEnabled && shadersLoaded && lightEnabled) {
                    // Set material uniforms using the material index
                    if (meshData.materialIndex >= 0 && meshData.materialIndex < (int)model.GetMaterials().size()) {
                        SetMaterialUniforms(model.GetMaterials()[meshData.materialIndex]);
                    }
                }
                
                DrawMesh(meshData.mesh, meshData.material, MatrixIdentity());
                
                // Draw per-mesh bounding box if enabled
                if (showBoundingBox) {
                    RenderMeshBoundingBox(meshData, worldTransform);
                }
            }
        }
        
        // End shader mode if PBR is enabled
        if (pbrEnabled && shadersLoaded && lightEnabled) {
            EndShaderMode();
        }
        
        if (wireframeMode) {
            rlDisableWireMode();
        }
        
        rlPopMatrix();
    }
    
    // Recursively render all children
    for (const auto& child : node->children) {
        RenderNodeHierarchy(child.get(), worldTransform, model, camera);
    }
}

void ModelRenderer::RenderWireframe(const Model& model) {
    if (!model.IsLoaded()) return;
    
    Matrix transform = model.GetTransformMatrix();
    
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));
    
    rlEnableWireMode();
    
    for (const auto& meshData : model.GetMeshes()) {
        DrawMesh(meshData.mesh, meshData.material, MatrixIdentity());
    }
    
    rlDisableWireMode();
    
    rlPopMatrix();
}

void ModelRenderer::RenderBoundingBox(const Model& model) {
    if (!model.IsLoaded()) return;
    
    Vector3 min = model.GetBoundingBoxMin();
    Vector3 max = model.GetBoundingBoxMax();
    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Scale(Vector3Add(min, max), 0.5f);
    
    Matrix transform = model.GetTransformMatrix();
    Vector3 transformedCenter = Vector3Transform(center, transform);
    
    // Transform size by scale component only
    Vector3 scale = model.GetScale();
    Vector3 transformedSize = {
        size.x * scale.x,
        size.y * scale.y,
        size.z * scale.z
    };
    
    DrawBoundingBox(
        BoundingBox{
            Vector3Subtract(transformedCenter, Vector3Scale(transformedSize, 0.5f)),
            Vector3Add(transformedCenter, Vector3Scale(transformedSize, 0.5f))
        },
        YELLOW
    );
}

void ModelRenderer::RenderMeshBoundingBox(const MeshData& meshData, const Matrix& transform) {
    // Transform mesh bounds by node's world transform
    Vector3 corners[8];
    Vector3 min = meshData.minBounds;
    Vector3 max = meshData.maxBounds;
    
    // Define 8 corners of bounding box
    corners[0] = {min.x, min.y, min.z};
    corners[1] = {max.x, min.y, min.z};
    corners[2] = {max.x, max.y, min.z};
    corners[3] = {min.x, max.y, min.z};
    corners[4] = {min.x, min.y, max.z};
    corners[5] = {max.x, min.y, max.z};
    corners[6] = {max.x, max.y, max.z};
    corners[7] = {min.x, max.y, max.z};
    
    // Transform all corners
    for (int i = 0; i < 8; i++) {
        corners[i] = Vector3Transform(corners[i], transform);
    }
    
    // Find new axis-aligned bounding box
    Vector3 newMin = corners[0];
    Vector3 newMax = corners[0];
    for (int i = 1; i < 8; i++) {
        newMin.x = fminf(newMin.x, corners[i].x);
        newMin.y = fminf(newMin.y, corners[i].y);
        newMin.z = fminf(newMin.z, corners[i].z);
        newMax.x = fmaxf(newMax.x, corners[i].x);
        newMax.y = fmaxf(newMax.y, corners[i].y);
        newMax.z = fmaxf(newMax.z, corners[i].z);
    }
    
    // Draw transformed bounding box
    BoundingBox transformedBox = {newMin, newMax};
    DrawBoundingBox(transformedBox, YELLOW);
}

void ModelRenderer::SetMaterialUniforms(const MaterialData& material) {
    // Bind textures to their respective texture units
    // (Sampler locations were already set during initialization)
    
    // Texture unit 0: albedoMap
    if (material.hasDiffuseTexture && material.diffuseTexture.id > 0) {
        rlActiveTextureSlot(0);
        rlEnableTexture(material.diffuseTexture.id);
    }
    
    // Texture unit 1: normalMap
    if (material.hasNormalTexture && material.normalTexture.id > 0) {
        rlActiveTextureSlot(1);
        rlEnableTexture(material.normalTexture.id);
    }
    
    // Texture unit 2: metallicMap
    if (material.hasMetalnessTexture && material.metalnessTexture.id > 0) {
        rlActiveTextureSlot(2);
        rlEnableTexture(material.metalnessTexture.id);
    }
    
    // Texture unit 3: roughnessMap
    if (material.hasRoughnessTexture && material.roughnessTexture.id > 0) {
        rlActiveTextureSlot(3);
        rlEnableTexture(material.roughnessTexture.id);
    }
    
    // Texture unit 4: aoMap
    if (material.hasAOTexture && material.aoTexture.id > 0) {
        rlActiveTextureSlot(4);
        rlEnableTexture(material.aoTexture.id);
    }
    
    // Texture unit 5: emissiveMap
    if (material.hasEmissiveTexture && material.emissiveTexture.id > 0) {
        rlActiveTextureSlot(5);
        rlEnableTexture(material.emissiveTexture.id);
    }
    
    // Set texture availability flags
    int hasAlbedo = material.hasDiffuseTexture ? 1 : 0;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "hasAlbedoMap"), &hasAlbedo, SHADER_UNIFORM_INT);
    
    int hasNormal = material.hasNormalTexture ? 1 : 0;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "hasNormalMap"), &hasNormal, SHADER_UNIFORM_INT);
    
    int hasMetallic = material.hasMetalnessTexture ? 1 : 0;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "hasMetallicMap"), &hasMetallic, SHADER_UNIFORM_INT);
    
    int hasRoughness = material.hasRoughnessTexture ? 1 : 0;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "hasRoughnessMap"), &hasRoughness, SHADER_UNIFORM_INT);
    
    int hasAO = material.hasAOTexture ? 1 : 0;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "hasAOMap"), &hasAO, SHADER_UNIFORM_INT);
    
    int hasEmissive = material.hasEmissiveTexture ? 1 : 0;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "hasEmissiveMap"), &hasEmissive, SHADER_UNIFORM_INT);
    
    // Set fallback material properties
    Vector3 diffuseColor = {material.diffuseColor.r / 255.0f, material.diffuseColor.g / 255.0f, material.diffuseColor.b / 255.0f};
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "colDiffuse"), &diffuseColor, SHADER_UNIFORM_VEC3);
    
    // Default metallic and roughness values if no texture
    float metallicValue = 0.0f;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "metallicValue"), &metallicValue, SHADER_UNIFORM_FLOAT);
    
    float roughnessValue = 0.5f;
    SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "roughnessValue"), &roughnessValue, SHADER_UNIFORM_FLOAT);
}

void ModelRenderer::RenderGrid(float size, int divisions) {
    DrawGrid(divisions, size);
}

} // namespace AAV

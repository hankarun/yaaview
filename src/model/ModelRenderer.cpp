#include "ModelRenderer.h"
#include "rlgl.h"
#include "raymath.h"
#include <iostream>

namespace AAV {

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
    , iblEnabled(true)
    , iblManager(std::make_unique<IBLManager>())
{
}

ModelRenderer::~ModelRenderer() {
    if (shadersLoaded) {
        UnloadShader(pbrShader);
    }
}

void ModelRenderer::Initialize() {
    // Load PBR shader
    pbrShader = LoadShader("shaders/pbr.vs", "shaders/pbr.fs");
    
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
    }
    
    // Initialize IBL system
    if (iblManager) {
        iblManager->Initialize();
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
            
            // Set IBL uniforms
            int iblEnabledInt = iblEnabled ? 1 : 0;
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "enableIBL"), &iblEnabledInt, SHADER_UNIFORM_INT);
            
            if (iblEnabled && iblManager && iblManager->IsLoaded()) {
                // Set IBL texture uniforms with explicit texture slots
                int irradianceLoc = GetShaderLocation(pbrShader, "irradianceMap");
                rlActiveTextureSlot(6);
                SetShaderValueTexture(pbrShader, irradianceLoc, iblManager->GetIrradianceMap());
                
                int prefilterLoc = GetShaderLocation(pbrShader, "prefilterMap");
                rlActiveTextureSlot(7);
                SetShaderValueTexture(pbrShader, prefilterLoc, iblManager->GetPrefilterMap());
                
                int brdfLoc = GetShaderLocation(pbrShader, "brdfLUT");
                rlActiveTextureSlot(8);
                SetShaderValueTexture(pbrShader, brdfLoc, iblManager->GetBRDFLUT());
            }
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
            
            // Set IBL uniforms
            int iblEnabledInt = iblEnabled ? 1 : 0;
            SetShaderValue(pbrShader, GetShaderLocation(pbrShader, "enableIBL"), &iblEnabledInt, SHADER_UNIFORM_INT);
            
            if (iblEnabled && iblManager && iblManager->IsLoaded()) {
                // Set IBL texture uniforms with explicit texture slots
                int irradianceLoc = GetShaderLocation(pbrShader, "irradianceMap");
                rlActiveTextureSlot(6);
                SetShaderValueTexture(pbrShader, irradianceLoc, iblManager->GetIrradianceMap());
                
                int prefilterLoc = GetShaderLocation(pbrShader, "prefilterMap");
                rlActiveTextureSlot(7);
                SetShaderValueTexture(pbrShader, prefilterLoc, iblManager->GetPrefilterMap());
                
                int brdfLoc = GetShaderLocation(pbrShader, "brdfLUT");
                rlActiveTextureSlot(8);
                SetShaderValueTexture(pbrShader, brdfLoc, iblManager->GetBRDFLUT());
            }
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
        (BoundingBox){
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
    DrawBoundingBox((BoundingBox){newMin, newMax}, YELLOW);
}

void ModelRenderer::SetMaterialUniforms(const MaterialData& material) {
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

void ModelRenderer::RenderSkybox(Camera3D camera, int viewportWidth, int viewportHeight) {
    if (iblManager && iblManager->IsLoaded()) {
        iblManager->RenderSkybox(camera, viewportWidth, viewportHeight);
    }
}

void ModelRenderer::SetSkyboxEnabled(bool enabled) {
    if (iblManager) {
        iblManager->SetSkyboxEnabled(enabled);
    }
}

bool ModelRenderer::GetSkyboxEnabled() const {
    if (iblManager) {
        return iblManager->IsSkyboxEnabled();
    }
    return false;
}

} // namespace AAV

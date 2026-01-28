#pragma once

#include "Model.h"
#include "raylib.h"

namespace AAV {

class ModelRenderer {
public:
    ModelRenderer();
    ~ModelRenderer();
    
    void Initialize();
    void Render(const Model& model, Camera3D camera);
    void RenderWireframe(const Model& model);
    void RenderBoundingBox(const Model& model);
    void RenderGrid(float size, int divisions);
    
    void SetWireframeMode(bool enabled) { wireframeMode = enabled; }
    bool GetWireframeMode() const { return wireframeMode; }
    
    void SetShowBoundingBox(bool enabled) { showBoundingBox = enabled; }
    bool GetShowBoundingBox() const { return showBoundingBox; }
    
    // Lighting control
    void SetLightEnabled(bool enabled) { lightEnabled = enabled; }
    void SetLightDirection(Vector3 dir) { lightDirection = dir; }
    void SetLightColor(Color color) { lightColor = color; }
    void SetLightIntensity(float intensity) { lightIntensity = intensity; }
    
    // Shader control
    void SetPBREnabled(bool enabled) { pbrEnabled = enabled; }
    bool GetPBREnabled() const { return pbrEnabled; }
    
    void SetNormalMappingEnabled(bool enabled) { normalMappingEnabled = enabled; }
    bool GetNormalMappingEnabled() const { return normalMappingEnabled; }
    
    bool IsShadersLoaded() const { return shadersLoaded; }
    
private:
    void RenderNodeHierarchy(NodeData* node, const Matrix& parentTransform, const Model& model, Camera3D camera);
    void RenderMeshBoundingBox(const MeshData& meshData, const Matrix& transform);
    void SetMaterialUniforms(const MaterialData& material);
    
    bool wireframeMode;
    bool showBoundingBox;
    
    // Lighting properties
    bool lightEnabled;
    Vector3 lightDirection;
    Color lightColor;
    float lightIntensity;
    
    // Shader system
    Shader pbrShader;
    bool shadersLoaded;
    bool pbrEnabled;
    bool normalMappingEnabled;
};

} // namespace AAV

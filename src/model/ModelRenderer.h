#pragma once

#include "Model.h"
#include "raylib.h"

namespace AAV {

class ModelRenderer {
public:
    ModelRenderer();
    ~ModelRenderer();
    
    void Render(const Model& model);
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
    
private:
    void RenderNodeHierarchy(NodeData* node, const Matrix& parentTransform, const Model& model);
    
    bool wireframeMode;
    bool showBoundingBox;
    
    // Lighting properties (for future shader implementation)
    bool lightEnabled;
    Vector3 lightDirection;
    Color lightColor;
    float lightIntensity;
};

} // namespace AAV

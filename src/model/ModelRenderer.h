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
    
private:
    void RenderNodeHierarchy(NodeData* node, const Matrix& parentTransform, const Model& model);
    
    bool wireframeMode;
    bool showBoundingBox;
};

} // namespace AAV

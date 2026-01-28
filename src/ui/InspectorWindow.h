#pragma once

#include "../model/Model.h"
#include <memory>

namespace AAV {

class InspectorWindow {
public:
    InspectorWindow();
    ~InspectorWindow();
    
    void Render(std::shared_ptr<Model> model, NodeData* selectedNode = nullptr);
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
    
private:
    void RenderModelInfo(Model& model);
    void RenderTransform(Model& model);
    void RenderMaterials(Model& model);
    void RenderHierarchy(Model& model);
    void RenderAnimations(Model& model);
    void RenderNodeInfo(NodeData* node, Model& model);
    void RenderNodeTransform(NodeData* node);
    void RenderNodeMeshes(NodeData* node, Model& model);
    
    bool visible;
    int selectedMaterial;
    int selectedAnimation;
};

} // namespace AAV

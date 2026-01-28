#pragma once

#include "../model/Model.h"
#include <memory>
#include <functional>

namespace AAV {

class HierarchyWindow {
public:
    HierarchyWindow();
    ~HierarchyWindow();
    
    void Render(std::shared_ptr<Model> model);
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
    
    // Callback when a node is selected
    std::function<void(NodeData*)> onNodeSelected;
    
private:
    void RenderNode(NodeData* node, int& nodeId);
    
    bool visible;
    NodeData* selectedNode;
};

} // namespace AAV

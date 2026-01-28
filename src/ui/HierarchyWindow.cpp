#include "HierarchyWindow.h"
#include "imgui.h"

namespace AAV {

HierarchyWindow::HierarchyWindow()
    : visible(true)
    , selectedNode(nullptr)
{
}

HierarchyWindow::~HierarchyWindow() {
}

void HierarchyWindow::Render(std::shared_ptr<Model> model) {
    if (!visible) return;
    
    ImGui::Begin("Hierarchy", &visible);
    
    if (!model || !model->IsLoaded()) {
        ImGui::Text("No model loaded");
        ImGui::End();
        return;
    }
    
    auto rootNode = model->GetRootNode();
    
    if (!rootNode) {
        ImGui::Text("No hierarchy data");
        ImGui::End();
        return;
    }
    
    ImGui::Text("Scene Graph");
    ImGui::Separator();
    
    // Render the hierarchy tree starting from root
    int nodeId = 0;
    RenderNode(rootNode.get(), nodeId);
    
    ImGui::End();
}

void HierarchyWindow::RenderNode(NodeData* node, int& nodeId) {
    if (!node) return;
    
    // Create unique ID for this tree node
    int currentId = nodeId++;
    
    // Build node label with mesh count if any
    std::string label = node->name;
    if (label.empty()) {
        label = "<unnamed>";
    }
    
    if (!node->meshIndices.empty()) {
        label += " [" + std::to_string(node->meshIndices.size()) + " mesh";
        if (node->meshIndices.size() > 1) label += "es";
        label += "]";
    }
    
    // Tree node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    
    // If this node has no children, make it a leaf
    if (node->children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    // Highlight if selected
    if (node == selectedNode) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    // Render the tree node
    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)currentId, flags, "%s", label.c_str());
    
    // Check if this node was clicked
    if (ImGui::IsItemClicked()) {
        selectedNode = node;
        if (onNodeSelected) {
            onNodeSelected(node);
        }
    }
    
    // Render children if the node is open
    if (nodeOpen && !node->children.empty()) {
        for (auto& child : node->children) {
            RenderNode(child.get(), nodeId);
        }
        ImGui::TreePop();
    }
}

} // namespace AAV

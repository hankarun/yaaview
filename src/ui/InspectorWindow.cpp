#include "InspectorWindow.h"
#include "imgui.h"
#include <sstream>
#include <iomanip>

namespace AAV {

InspectorWindow::InspectorWindow()
    : visible(true)
    , selectedMaterial(0)
    , selectedAnimation(0)
{
}

InspectorWindow::~InspectorWindow() {
}

void InspectorWindow::Render(std::shared_ptr<Model> model) {
    if (!visible) return;
    
    ImGui::Begin("Inspector", &visible);
    
    if (!model || !model->IsLoaded()) {
        ImGui::Text("No model loaded");
        ImGui::Text("Use File > Open Model to load a 3D model");
        ImGui::End();
        return;
    }
    
    // Render all sections with collapsible headers
    if (ImGui::CollapsingHeader("Model Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderModelInfo(*model);
    }
    
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderTransform(*model);
    }
    
    if (ImGui::CollapsingHeader("Materials")) {
        RenderMaterials(*model);
    }
    
    if (ImGui::CollapsingHeader("Hierarchy")) {
        RenderHierarchy(*model);
    }
    
    if (ImGui::CollapsingHeader("Animations")) {
        RenderAnimations(*model);
    }
    
    ImGui::End();
}

void InspectorWindow::RenderModelInfo(Model& model) {
    ImGui::Indent();
    
    ImGui::Text("Filename: %s", model.GetFilename().c_str());
    ImGui::Separator();
    
    ImGui::Text("Meshes: %d", model.GetMeshCount());
    ImGui::Text("Materials: %d", model.GetMaterialCount());
    ImGui::Text("Animations: %d", model.GetAnimationCount());
    
    ImGui::Separator();
    
    ImGui::Text("Total Vertices: %d", model.GetTotalVertices());
    ImGui::Text("Total Faces: %d", model.GetTotalFaces());
    
    ImGui::Separator();
    
    Vector3 size = model.GetBoundingBoxSize();
    ImGui::Text("Bounding Box:");
    ImGui::BulletText("Size: (%.2f, %.2f, %.2f)", size.x, size.y, size.z);
    
    Vector3 min = model.GetBoundingBoxMin();
    Vector3 max = model.GetBoundingBoxMax();
    ImGui::BulletText("Min: (%.2f, %.2f, %.2f)", min.x, min.y, min.z);
    ImGui::BulletText("Max: (%.2f, %.2f, %.2f)", max.x, max.y, max.z);
    
    ImGui::Unindent();
}

void InspectorWindow::RenderTransform(Model& model) {
    ImGui::Indent();
    
    Vector3 pos = model.GetPosition();
    Vector3 rot = model.GetRotation();
    Vector3 scl = model.GetScale();
    
    bool changed = false;
    
    ImGui::Text("Position:");
    if (ImGui::DragFloat3("##Position", &pos.x, 0.1f)) {
        model.SetPosition(pos);
        changed = true;
    }
    
    ImGui::Text("Rotation:");
    if (ImGui::DragFloat3("##Rotation", &rot.x, 1.0f, -180.0f, 180.0f)) {
        model.SetRotation(rot);
        changed = true;
    }
    
    ImGui::Text("Scale:");
    static bool uniformScale = true;
    ImGui::Checkbox("Uniform", &uniformScale);
    ImGui::SameLine();
    
    if (uniformScale) {
        float scale = scl.x;
        if (ImGui::DragFloat("##Scale", &scale, 0.01f, 0.01f, 100.0f)) {
            model.SetScale({scale, scale, scale});
            changed = true;
        }
    } else {
        if (ImGui::DragFloat3("##Scale", &scl.x, 0.01f, 0.01f, 100.0f)) {
            model.SetScale(scl);
            changed = true;
        }
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
        model.ResetTransform();
    }
    
    ImGui::Unindent();
}

void InspectorWindow::RenderMaterials(Model& model) {
    ImGui::Indent();
    
    const auto& materials = model.GetMaterials();
    
    if (materials.empty()) {
        ImGui::Text("No materials");
        ImGui::Unindent();
        return;
    }
    
    ImGui::Text("Material Count: %d", (int)materials.size());
    ImGui::Separator();
    
    for (size_t i = 0; i < materials.size(); i++) {
        const MaterialData& mat = materials[i];
        
        std::string label = mat.name.empty() ? 
            "Material " + std::to_string(i) : mat.name;
        
        if (ImGui::TreeNode((void*)(intptr_t)i, "%s", label.c_str())) {
            ImVec4 diffuse = ImVec4(
                mat.diffuseColor.r / 255.0f,
                mat.diffuseColor.g / 255.0f,
                mat.diffuseColor.b / 255.0f,
                1.0f
            );
            ImGui::ColorEdit3("Diffuse", (float*)&diffuse, ImGuiColorEditFlags_NoInputs);
            
            ImVec4 specular = ImVec4(
                mat.specularColor.r / 255.0f,
                mat.specularColor.g / 255.0f,
                mat.specularColor.b / 255.0f,
                1.0f
            );
            ImGui::ColorEdit3("Specular", (float*)&specular, ImGuiColorEditFlags_NoInputs);
            
            ImGui::Text("Shininess: %.2f", mat.shininess);
            
            if (!mat.diffuseTexturePath.empty()) {
                ImGui::Text("Diffuse Texture: %s", mat.diffuseTexturePath.c_str());
            }
            
            ImGui::TreePop();
        }
    }
    
    ImGui::Unindent();
}

void InspectorWindow::RenderHierarchy(Model& model) {
    ImGui::Indent();
    
    auto rootNode = model.GetRootNode();
    
    if (!rootNode) {
        ImGui::Text("No hierarchy data");
        ImGui::Unindent();
        return;
    }
    
    // Render root node
    ImGui::Text("Root: %s", rootNode->name.c_str());
    
    if (!rootNode->meshIndices.empty()) {
        ImGui::BulletText("Meshes: %d", (int)rootNode->meshIndices.size());
    }
    
    if (!rootNode->children.empty()) {
        ImGui::BulletText("Children: %d", (int)rootNode->children.size());
        
        // TODO: Render full hierarchy tree
        ImGui::TextDisabled("(Full hierarchy tree not yet implemented)");
    }
    
    ImGui::Unindent();
}

void InspectorWindow::RenderAnimations(Model& model) {
    ImGui::Indent();
    
    const auto& animations = model.GetAnimations();
    
    if (animations.empty()) {
        ImGui::Text("No animations");
        ImGui::Unindent();
        return;
    }
    
    ImGui::Text("Animation Count: %d", (int)animations.size());
    ImGui::Separator();
    
    for (size_t i = 0; i < animations.size(); i++) {
        const AnimationData& anim = animations[i];
        
        std::string label = anim.name.empty() ? 
            "Animation " + std::to_string(i) : anim.name;
        
        if (ImGui::TreeNode((void*)(intptr_t)i, "%s", label.c_str())) {
            ImGui::Text("Duration: %.2f seconds", anim.duration / anim.ticksPerSecond);
            ImGui::Text("Ticks per second: %.2f", anim.ticksPerSecond);
            
            // TODO: Add playback controls
            ImGui::TextDisabled("(Animation playback not yet implemented)");
            
            ImGui::TreePop();
        }
    }
    
    ImGui::Unindent();
}

} // namespace AAV

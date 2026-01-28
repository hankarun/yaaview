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

void InspectorWindow::Render(std::shared_ptr<Model> model, NodeData* selectedNode) {
    if (!visible) return;
    
    ImGui::Begin("Inspector", &visible);
    
    if (!model || !model->IsLoaded()) {
        ImGui::Text("No model loaded");
        ImGui::Text("Use File > Open Model to load a 3D model");
        ImGui::End();
        return;
    }
    
    // If a node is selected, show node-specific information
    if (selectedNode) {
        if (ImGui::CollapsingHeader("Node Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderNodeInfo(selectedNode, *model);
        }
        
        if (ImGui::CollapsingHeader("Node Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderNodeTransform(selectedNode);
        }
        
        if (!selectedNode->meshIndices.empty()) {
            if (ImGui::CollapsingHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderNodeMeshes(selectedNode, *model);
            }
        }
        
        ImGui::Separator();
        ImGui::TextDisabled("Model-wide properties:");
    }
    
    // Render model-wide sections
    if (ImGui::CollapsingHeader("Transform")) {
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

void InspectorWindow::RenderNodeInfo(NodeData* node, Model& model) {
    ImGui::Indent();
    
    ImGui::Text("Node Name: %s", node->name.empty() ? "<unnamed>" : node->name.c_str());
    
    if (node->parent) {
        ImGui::Text("Parent: %s", node->parent->name.empty() ? "<unnamed>" : node->parent->name.c_str());
    } else {
        ImGui::Text("Parent: <root>");
    }
    
    ImGui::Text("Children: %d", (int)node->children.size());
    ImGui::Text("Attached Meshes: %d", (int)node->meshIndices.size());
    
    ImGui::Unindent();
}

void InspectorWindow::RenderNodeTransform(NodeData* node) {
    ImGui::Indent();
    
    // Extract transform information from the node's matrix
    // For now, we'll display the matrix values
    ImGui::Text("Local Transform Matrix:");
    
    Matrix& m = node->transform;
    ImGui::TextDisabled("[ %.2f %.2f %.2f %.2f ]", m.m0, m.m4, m.m8, m.m12);
    ImGui::TextDisabled("[ %.2f %.2f %.2f %.2f ]", m.m1, m.m5, m.m9, m.m13);
    ImGui::TextDisabled("[ %.2f %.2f %.2f %.2f ]", m.m2, m.m6, m.m10, m.m14);
    ImGui::TextDisabled("[ %.2f %.2f %.2f %.2f ]", m.m3, m.m7, m.m11, m.m15);
    
    ImGui::Separator();
    
    // Extract position from matrix (translation component)
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", m.m12, m.m13, m.m14);
    
    // Note: Full decomposition would require more complex math
    ImGui::TextDisabled("(Full transform decomposition coming soon)");
    
    ImGui::Unindent();
}

void InspectorWindow::RenderNodeMeshes(NodeData* node, Model& model) {
    ImGui::Indent();
    
    const auto& meshes = model.GetMeshes();
    
    for (size_t i = 0; i < node->meshIndices.size(); i++) {
        int meshIndex = node->meshIndices[i];
        
        if (meshIndex < 0 || meshIndex >= (int)meshes.size()) {
            ImGui::Text("Mesh %d: <invalid index>", meshIndex);
            continue;
        }
        
        const MeshData& meshData = meshes[meshIndex];
        
        std::string label = meshData.name.empty() ? 
            "Mesh " + std::to_string(meshIndex) : meshData.name;
        
        if (ImGui::TreeNode((void*)(intptr_t)(meshIndex + 1000), "%s", label.c_str())) {
            ImGui::Text("Vertices: %d", meshData.mesh.vertexCount);
            ImGui::Text("Triangles: %d", meshData.mesh.triangleCount);
            ImGui::Text("Material Index: %d", meshData.materialIndex);
            
            ImGui::Separator();
            ImGui::Text("Bounding Box:");
            ImGui::BulletText("Min: (%.2f, %.2f, %.2f)", 
                meshData.minBounds.x, meshData.minBounds.y, meshData.minBounds.z);
            ImGui::BulletText("Max: (%.2f, %.2f, %.2f)", 
                meshData.maxBounds.x, meshData.maxBounds.y, meshData.maxBounds.z);
            
            // Show material properties
            const auto& materials = model.GetMaterials();
            if (meshData.materialIndex >= 0 && meshData.materialIndex < (int)materials.size()) {
                ImGui::Separator();
                const MaterialData& mat = materials[meshData.materialIndex];
                
                ImGui::Text("Material: %s", mat.name.empty() ? "<unnamed>" : mat.name.c_str());
                
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
                
                // Helper lambda for rendering texture info with preview
                auto renderTextureInfo = [](const char* label, const std::string& path, 
                                             bool hasTexture, const Texture2D& texture) {
                    if (!path.empty()) {
                        ImGui::Separator();
                        ImGui::Text("%s:", label);
                        ImGui::Indent();
                        ImGui::BulletText("Path: %s", path.c_str());
                        
                        if (hasTexture && texture.id != 0) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[Loaded]");
                            ImGui::BulletText("Size: %dx%d", texture.width, texture.height);
                            
                            // Show thumbnail preview
                            float previewSize = 128.0f;
                            float aspectRatio = (float)texture.width / (float)texture.height;
                            ImVec2 imageSize;
                            if (aspectRatio > 1.0f) {
                                imageSize = ImVec2(previewSize, previewSize / aspectRatio);
                            } else {
                                imageSize = ImVec2(previewSize * aspectRatio, previewSize);
                            }
                            
                            ImGui::Image((void*)(intptr_t)texture.id, imageSize);
                        } else {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[Failed]");
                        }
                        
                        ImGui::Unindent();
                    }
                };
                
                // Render all texture types
                renderTextureInfo("Diffuse", mat.diffuseTexturePath, 
                                  mat.hasDiffuseTexture, mat.diffuseTexture);
                renderTextureInfo("Specular", mat.specularTexturePath, 
                                  mat.hasSpecularTexture, mat.specularTexture);
                renderTextureInfo("Normal", mat.normalTexturePath, 
                                  mat.hasNormalTexture, mat.normalTexture);
                renderTextureInfo("Metalness", mat.metalnessTexturePath, 
                                  mat.hasMetalnessTexture, mat.metalnessTexture);
                renderTextureInfo("Roughness", mat.roughnessTexturePath, 
                                  mat.hasRoughnessTexture, mat.roughnessTexture);
                renderTextureInfo("Ambient Occlusion", mat.aoTexturePath, 
                                  mat.hasAOTexture, mat.aoTexture);
                renderTextureInfo("Emissive", mat.emissiveTexturePath, 
                                  mat.hasEmissiveTexture, mat.emissiveTexture);
            }
            
            ImGui::TreePop();
        }
    }
    
    ImGui::Unindent();
}

} // namespace AAV

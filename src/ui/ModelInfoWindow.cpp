#include "ModelInfoWindow.h"
#include "imgui.h"

namespace AAV {

ModelInfoWindow::ModelInfoWindow()
    : visible(true)
{
}

ModelInfoWindow::~ModelInfoWindow() {
}

void ModelInfoWindow::Render(std::shared_ptr<Model> model) {
    if (!visible) return;
    
    ImGui::Begin("Model Info", &visible);
    
    if (!model || !model->IsLoaded()) {
        ImGui::Text("No model loaded");
        ImGui::Text("Use File > Open Model to load a 3D model");
        ImGui::End();
        return;
    }
    
    ImGui::Text("Filename: %s", model->GetFilename().c_str());
    ImGui::Separator();
    
    ImGui::Text("Meshes: %d", model->GetMeshCount());
    ImGui::Text("Materials: %d", model->GetMaterialCount());
    ImGui::Text("Textures: %d", model->GetTotalTexturesLoaded());
    ImGui::Text("Animations: %d", model->GetAnimationCount());
    
    ImGui::Separator();
    
    ImGui::Text("Total Vertices: %d", model->GetTotalVertices());
    ImGui::Text("Total Faces: %d", model->GetTotalFaces());
    
    ImGui::Separator();
    
    Vector3 size = model->GetBoundingBoxSize();
    ImGui::Text("Bounding Box:");
    ImGui::BulletText("Size: (%.2f, %.2f, %.2f)", size.x, size.y, size.z);
    
    Vector3 min = model->GetBoundingBoxMin();
    Vector3 max = model->GetBoundingBoxMax();
    ImGui::BulletText("Min: (%.2f, %.2f, %.2f)", min.x, min.y, min.z);
    ImGui::BulletText("Max: (%.2f, %.2f, %.2f)", max.x, max.y, max.z);
    
    ImGui::End();
}

} // namespace AAV

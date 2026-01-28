#include "SettingsWindow.h"
#include "imgui.h"

namespace AAV {

SettingsWindow::SettingsWindow()
    : showModal(false)
{
}

SettingsWindow::~SettingsWindow() {
}

void SettingsWindow::Render(SceneWindow* sceneWindow) {
    if (!sceneWindow) return;
    
    // Open modal if requested
    if (showModal) {
        ImGui::OpenPopup("Settings");
        showModal = false;
    }
    
    // Center the modal window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, 
                                     viewport->WorkPos.y + viewport->WorkSize.y * 0.5f), 
                            ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_Appearing);
    
    if (!ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_NoResize)) {
        return;
    }
    
    // Grid Settings Section
    if (ImGui::CollapsingHeader("Grid Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool gridEnabled = sceneWindow->GetGridEnabled();
        if (ImGui::Checkbox("Show Grid", &gridEnabled)) {
            sceneWindow->SetGridEnabled(gridEnabled);
        }
        
        if (gridEnabled) {
            float gridSize = sceneWindow->GetGridSize();
            if (ImGui::SliderFloat("Grid Size", &gridSize, 0.5f, 10.0f, "%.1f")) {
                sceneWindow->SetGridSize(gridSize);
            }
            
            int gridDivisions = sceneWindow->GetGridDivisions();
            if (ImGui::SliderInt("Grid Divisions", &gridDivisions, 5, 50)) {
                sceneWindow->SetGridDivisions(gridDivisions);
            }
        }
        
        ImGui::Spacing();
    }
    
    // Render Settings Section
    if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        Color bgColor = sceneWindow->GetBackgroundColor();
        float color[3] = {
            bgColor.r / 255.0f,
            bgColor.g / 255.0f,
            bgColor.b / 255.0f
        };
        
        if (ImGui::ColorEdit3("Background Color", color)) {
            sceneWindow->SetBackgroundColor(Color{
                (unsigned char)(color[0] * 255),
                (unsigned char)(color[1] * 255),
                (unsigned char)(color[2] * 255),
                255
            });
        }
        
        ImGui::Spacing();
        
        bool wireframe = sceneWindow->GetRenderer().GetWireframeMode();
        if (ImGui::Checkbox("Wireframe Mode", &wireframe)) {
            sceneWindow->GetRenderer().SetWireframeMode(wireframe);
        }
        
        bool showBoundingBox = sceneWindow->GetRenderer().GetShowBoundingBox();
        if (ImGui::Checkbox("Show Bounding Box", &showBoundingBox)) {
            sceneWindow->GetRenderer().SetShowBoundingBox(showBoundingBox);
        }
        
        ImGui::Spacing();
    }
    
    // Shader Settings Section
    if (ImGui::CollapsingHeader("Shader Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& renderer = sceneWindow->GetRenderer();
        
        // Show shader status
        if (renderer.IsShadersLoaded()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "PBR Shaders: Loaded");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), "PBR Shaders: Not Loaded");
            ImGui::TextWrapped("Shaders failed to load. Using default rendering.");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Only show shader options if shaders are loaded
        if (renderer.IsShadersLoaded()) {
            bool pbrEnabled = renderer.GetPBREnabled();
            if (ImGui::Checkbox("Enable PBR Rendering", &pbrEnabled)) {
                renderer.SetPBREnabled(pbrEnabled);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Physically Based Rendering with metallic-roughness workflow");
                ImGui::EndTooltip();
            }
            
            bool normalMapping = renderer.GetNormalMappingEnabled();
            if (ImGui::Checkbox("Enable Normal Mapping", &normalMapping)) {
                renderer.SetNormalMappingEnabled(normalMapping);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Use normal maps for surface detail");
                ImGui::EndTooltip();
            }
            
            bool iblEnabled = renderer.GetIBLEnabled();
            if (ImGui::Checkbox("Enable IBL (Image-Based Lighting)", &iblEnabled)) {
                renderer.SetIBLEnabled(iblEnabled);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Use environment maps for ambient lighting and reflections");
                ImGui::EndTooltip();
            }
            
            if (iblEnabled) {
                ImGui::Indent();
                
                bool skyboxEnabled = renderer.GetSkyboxEnabled();
                if (ImGui::Checkbox("Show Skybox", &skyboxEnabled)) {
                    renderer.SetSkyboxEnabled(skyboxEnabled);
                }
                
                ImGui::Unindent();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::TextWrapped("Shader Features:");
            ImGui::BulletText("Metallic-Roughness PBR workflow");
            ImGui::BulletText("Directional lighting");
            ImGui::BulletText("Normal mapping");
            ImGui::BulletText("Image-Based Lighting (IBL)");
            ImGui::BulletText("Emissive materials");
            ImGui::BulletText("Ambient occlusion");
            ImGui::BulletText("HDR tone mapping");
            ImGui::BulletText("Gamma correction");
        }
        
        ImGui::Spacing();
    }
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Bottom buttons
    if (ImGui::Button("Reset All Settings", ImVec2(240, 0))) {
        sceneWindow->SetGridEnabled(true);
        sceneWindow->SetGridSize(1.0f);
        sceneWindow->SetGridDivisions(10);
        sceneWindow->SetBackgroundColor(Color{45, 45, 48, 255});
        sceneWindow->GetRenderer().SetWireframeMode(false);
        sceneWindow->GetRenderer().SetShowBoundingBox(false);
        sceneWindow->GetRenderer().SetPBREnabled(true);
        sceneWindow->GetRenderer().SetNormalMappingEnabled(true);
        sceneWindow->GetRenderer().SetIBLEnabled(true);
        sceneWindow->GetRenderer().SetSkyboxEnabled(true);
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Close", ImVec2(240, 0))) {
        ImGui::CloseCurrentPopup();
    }
    
    ImGui::EndPopup();
}

} // namespace AAV

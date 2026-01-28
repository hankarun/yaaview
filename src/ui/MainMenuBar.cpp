#include "MainMenuBar.h"
#include "imgui.h"

namespace AAV {

MainMenuBar::MainMenuBar()
    : inspectorVisible(true)
    , sceneVisible(true)
    , showAboutPopup(false)
    , showControlsPopup(false)
{
}

MainMenuBar::~MainMenuBar() {
}

void MainMenuBar::Render() {
    if (ImGui::BeginMainMenuBar()) {
        // File Menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Model...", "Ctrl+O")) {
                if (onOpenFile) onOpenFile();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                if (onExit) onExit();
            }
            
            ImGui::EndMenu();
        }
        
        // Edit Menu
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Preferences", nullptr, false, false)) {
                // TODO: Implement preferences
            }
            ImGui::EndMenu();
        }
        
        // View Menu
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Inspector", nullptr, inspectorVisible)) {
                inspectorVisible = !inspectorVisible;
                if (onToggleInspector) onToggleInspector();
            }
            
            if (ImGui::MenuItem("Scene", nullptr, sceneVisible)) {
                sceneVisible = !sceneVisible;
                if (onToggleScene) onToggleScene();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Reset Camera", "R")) {
                if (onResetCamera) onResetCamera();
            }
            
            ImGui::EndMenu();
        }
        
        // Window Menu
        if (ImGui::BeginMenu("Window")) {
            ImGui::Text("Layout Presets:");
            ImGui::Separator();
            
            if (ImGui::MenuItem("Classic")) {
                if (onLoadLayout) onLoadLayout(0);
            }
            
            if (ImGui::MenuItem("Wide Inspector")) {
                if (onLoadLayout) onLoadLayout(1);
            }
            
            if (ImGui::MenuItem("Full Scene")) {
                if (onLoadLayout) onLoadLayout(2);
            }
            
            ImGui::EndMenu();
        }
        
        // Help Menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                showAboutPopup = true;
            }
            
            if (ImGui::MenuItem("Controls")) {
                showControlsPopup = true;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("ImGui Demo")) {
                if (onToggleImGuiDemo) onToggleImGuiDemo();
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // About Popup
    if (showAboutPopup) {
        ImGui::OpenPopup("About AAV");
        showAboutPopup = false;
    }
    
    if (ImGui::BeginPopupModal("About AAV", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Assimp Advanced Viewer (AAV)");
        ImGui::Text("Version 1.0.0");
        ImGui::Separator();
        ImGui::Text("A 3D model viewer built with:");
        ImGui::BulletText("Raylib 5.5");
        ImGui::BulletText("Dear ImGui (Docking)");
        ImGui::BulletText("Assimp");
        ImGui::Separator();
        
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // Controls Popup
    if (showControlsPopup) {
        ImGui::OpenPopup("Controls");
        showControlsPopup = false;
    }
    
    if (ImGui::BeginPopupModal("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Camera Controls:");
        ImGui::Separator();
        ImGui::BulletText("Left Mouse + Drag: Rotate camera");
        ImGui::BulletText("Right Mouse + Drag: Pan camera");
        ImGui::BulletText("Mouse Wheel: Zoom in/out");
        ImGui::BulletText("R: Reset camera");
        ImGui::Separator();
        ImGui::Text("Keyboard Shortcuts:");
        ImGui::Separator();
        ImGui::BulletText("Ctrl+O: Open model");
        ImGui::BulletText("Alt+F4: Exit");
        ImGui::Separator();
        
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

} // namespace AAV

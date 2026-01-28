#include "LightWindow.h"
#include "imgui.h"

namespace AAV {

LightWindow::LightWindow()
    : visible(true)
{
}

LightWindow::~LightWindow() {
}

void LightWindow::Render(SceneWindow* sceneWindow) {
    if (!visible || !sceneWindow) return;
    
    ImGui::Begin("Lighting", &visible);
    
    ImGui::TextWrapped("Note: Lighting controls are configured but require custom shaders for full effect. Current settings are stored for future implementation.");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Directional Light");
    ImGui::Separator();
    
    // Enable/Disable light
    bool lightEnabled = sceneWindow->GetLightEnabled();
    if (ImGui::Checkbox("Enable Light", &lightEnabled)) {
        sceneWindow->SetLightEnabled(lightEnabled);
    }
    
    ImGui::Spacing();
    
    // Light Color
    Color lightColor = sceneWindow->GetLightColor();
    float color[3] = {
        lightColor.r / 255.0f,
        lightColor.g / 255.0f,
        lightColor.b / 255.0f
    };
    
    if (ImGui::ColorEdit3("Light Color", color)) {
        sceneWindow->SetLightColor(Color{
            (unsigned char)(color[0] * 255),
            (unsigned char)(color[1] * 255),
            (unsigned char)(color[2] * 255),
            255
        });
    }
    
    ImGui::Spacing();
    
    // Light Direction
    Vector3 lightDir = sceneWindow->GetLightDirection();
    float direction[3] = { lightDir.x, lightDir.y, lightDir.z };
    
    ImGui::Text("Light Direction");
    if (ImGui::SliderFloat("X##LightDir", &direction[0], -1.0f, 1.0f)) {
        sceneWindow->SetLightDirection(Vector3{direction[0], direction[1], direction[2]});
    }
    if (ImGui::SliderFloat("Y##LightDir", &direction[1], -1.0f, 1.0f)) {
        sceneWindow->SetLightDirection(Vector3{direction[0], direction[1], direction[2]});
    }
    if (ImGui::SliderFloat("Z##LightDir", &direction[2], -1.0f, 1.0f)) {
        sceneWindow->SetLightDirection(Vector3{direction[0], direction[1], direction[2]});
    }
    
    ImGui::Spacing();
    
    // Light Intensity
    float intensity = sceneWindow->GetLightIntensity();
    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 2.0f)) {
        sceneWindow->SetLightIntensity(intensity);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Reset button
    if (ImGui::Button("Reset Light", ImVec2(-1, 0))) {
        sceneWindow->SetLightEnabled(true);
        sceneWindow->SetLightDirection(Vector3{-0.5f, -1.0f, -0.3f});
        sceneWindow->SetLightColor(WHITE);
        sceneWindow->SetLightIntensity(1.0f);
    }
    
    ImGui::End();
}

} // namespace AAV

#pragma once

#include "raylib.h"
#include "../model/Model.h"
#include "../model/ModelRenderer.h"
#include <memory>

namespace AAV {

class SceneWindow {
public:
    SceneWindow();
    ~SceneWindow();
    
    void Initialize();
    void Render(std::shared_ptr<Model> model);
    void Cleanup();
    
    void ResetCamera();
    void FrameModel(std::shared_ptr<Model> model);
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
    
    ModelRenderer& GetRenderer() { return renderer; }
    
    // Lighting control
    void SetLightEnabled(bool enabled) { lightEnabled = enabled; }
    bool GetLightEnabled() const { return lightEnabled; }
    void SetLightDirection(Vector3 dir) { lightDirection = dir; }
    Vector3 GetLightDirection() const { return lightDirection; }
    void SetLightColor(Color color) { lightColor = color; }
    Color GetLightColor() const { return lightColor; }
    void SetLightIntensity(float intensity) { lightIntensity = intensity; }
    float GetLightIntensity() const { return lightIntensity; }
    
    // Grid control
    void SetGridEnabled(bool enabled) { gridEnabled = enabled; }
    bool GetGridEnabled() const { return gridEnabled; }
    void SetGridSize(float size) { gridSize = size; }
    float GetGridSize() const { return gridSize; }
    void SetGridDivisions(int divisions) { gridDivisions = divisions; }
    int GetGridDivisions() const { return gridDivisions; }
    
    // Render settings
    void SetBackgroundColor(Color color) { backgroundColor = color; }
    Color GetBackgroundColor() const { return backgroundColor; }
    
private:
    void UpdateCamera();
    void RenderScene(std::shared_ptr<Model> model);
    
    Camera3D camera;
    RenderTexture2D renderTexture;
    ModelRenderer renderer;
    
    bool visible;
    bool initialized;
    
    // Camera control state
    Vector2 lastMousePos;
    bool isDragging;
    bool isPanning;
    float cameraDistance;
    float cameraRotationX;
    float cameraRotationY;
    Vector3 cameraTarget;
    
    // Lighting properties
    bool lightEnabled;
    Vector3 lightDirection;
    Color lightColor;
    float lightIntensity;
    
    // Grid properties
    bool gridEnabled;
    float gridSize;
    int gridDivisions;
    
    // Render settings
    Color backgroundColor;
};

} // namespace AAV

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
};

} // namespace AAV

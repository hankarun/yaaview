#include "SceneWindow.h"
#include "imgui.h"
#include "rlgl.h"
#include <cmath>

namespace AAV {

SceneWindow::SceneWindow()
    : visible(true)
    , initialized(false)
    , isDragging(false)
    , isPanning(false)
    , cameraDistance(10.0f)
    , cameraRotationX(45.0f)
    , cameraRotationY(45.0f)
    , cameraTarget({0.0f, 0.0f, 0.0f})
    , lastMousePos({0.0f, 0.0f})
    , lightEnabled(true)
    , lightDirection({-0.5f, -1.0f, -0.3f})
    , lightColor(WHITE)
    , lightIntensity(1.0f)
    , gridEnabled(true)
    , gridSize(1.0f)
    , gridDivisions(10)
    , backgroundColor({45, 45, 48, 255})
{
}

SceneWindow::~SceneWindow() {
    Cleanup();
}

void SceneWindow::Initialize() {
    if (initialized) return;
    
    // Initialize camera
    camera.position = {10.0f, 10.0f, 10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Create render texture for the 3D scene
    renderTexture = LoadRenderTexture(1280, 720);
    
    // Initialize renderer (load shaders)
    renderer.Initialize();
    
    initialized = true;
}

void SceneWindow::Cleanup() {
    if (initialized) {
        UnloadRenderTexture(renderTexture);
        initialized = false;
    }
}

void SceneWindow::ResetCamera() {
    cameraDistance = 10.0f;
    cameraRotationX = 45.0f;
    cameraRotationY = 45.0f;
    cameraTarget = {0.0f, 0.0f, 0.0f};
    UpdateCamera();
}

void SceneWindow::FrameModel(std::shared_ptr<Model> model) {
    if (!model || !model->IsLoaded()) return;
    
    // Get model bounds
    Vector3 size = model->GetBoundingBoxSize();
    Vector3 center = model->GetCenter();
    
    // Calculate distance to fit model in view
    float maxSize = fmaxf(fmaxf(size.x, size.y), size.z);
    cameraDistance = maxSize * 2.0f;
    
    // Set camera target to model center
    cameraTarget = center;
    
    UpdateCamera();
}

void SceneWindow::UpdateCamera() {
    // Convert spherical coordinates to cartesian
    float rotXRad = cameraRotationX * DEG2RAD;
    float rotYRad = cameraRotationY * DEG2RAD;
    
    camera.position.x = cameraTarget.x + cameraDistance * cosf(rotXRad) * cosf(rotYRad);
    camera.position.y = cameraTarget.y + cameraDistance * sinf(rotXRad);
    camera.position.z = cameraTarget.z + cameraDistance * cosf(rotXRad) * sinf(rotYRad);
    
    camera.target = cameraTarget;
}

void SceneWindow::Render(std::shared_ptr<Model> model) {
    if (!visible) return;
    if (!initialized) Initialize();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene", &visible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    // Get available region for rendering
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    // Resize render texture if needed
    if (viewportSize.x > 0 && viewportSize.y > 0) {
        if (renderTexture.texture.width != (int)viewportSize.x || 
            renderTexture.texture.height != (int)viewportSize.y) {
            UnloadRenderTexture(renderTexture);
            renderTexture = LoadRenderTexture((int)viewportSize.x, (int)viewportSize.y);
        }
    }
    
    // Handle mouse input for camera control (only when window is focused and hovered)
    if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 mouseDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        
        // Left mouse button - Rotate camera
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!isDragging) {
                isDragging = true;
                lastMousePos = {mousePos.x, mousePos.y};
            }
            
            float deltaX = mousePos.x - lastMousePos.x;
            float deltaY = mousePos.y - lastMousePos.y;
            
            cameraRotationY += deltaX * 0.5f;
            cameraRotationX -= deltaY * 0.5f;
            
            // Clamp vertical rotation
            if (cameraRotationX > 89.0f) cameraRotationX = 89.0f;
            if (cameraRotationX < -89.0f) cameraRotationX = -89.0f;
            
            lastMousePos = {mousePos.x, mousePos.y};
            UpdateCamera();
        } else {
            isDragging = false;
        }
        
        // Right mouse button - Pan camera
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            if (!isPanning) {
                isPanning = true;
                lastMousePos = {mousePos.x, mousePos.y};
            }
            
            float deltaX = mousePos.x - lastMousePos.x;
            float deltaY = mousePos.y - lastMousePos.y;
            
            // Calculate pan vectors
            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
            Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));
            
            float panSpeed = cameraDistance * 0.001f;
            Vector3 panOffset = Vector3Add(
                Vector3Scale(right, -deltaX * panSpeed),
                Vector3Scale(up, deltaY * panSpeed)
            );
            
            cameraTarget = Vector3Add(cameraTarget, panOffset);
            
            lastMousePos = {mousePos.x, mousePos.y};
            UpdateCamera();
        } else {
            isPanning = false;
        }
        
        // Mouse wheel - Zoom
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            cameraDistance -= wheel * cameraDistance * 0.1f;
            if (cameraDistance < 0.1f) cameraDistance = 0.1f;
            UpdateCamera();
        }
    } else {
        isDragging = false;
        isPanning = false;
    }
    
    // Render 3D scene to texture
    RenderScene(model);
    
    // Display the rendered texture in ImGui
    ImGui::Image(
        (void*)(intptr_t)renderTexture.texture.id,
        viewportSize,
        ImVec2(0, 1),  // UV coordinates flipped for OpenGL
        ImVec2(1, 0)
    );
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void SceneWindow::RenderScene(std::shared_ptr<Model> model) {
    BeginTextureMode(renderTexture);
    
    ClearBackground(backgroundColor);
    
    BeginMode3D(camera);
        
    // Sync lighting settings to renderer
    renderer.SetLightEnabled(lightEnabled);
    renderer.SetLightDirection(lightDirection);
    renderer.SetLightColor(lightColor);
    renderer.SetLightIntensity(lightIntensity);
    
    // Render grid if enabled
    if (gridEnabled) {
        renderer.RenderGrid(gridSize, gridDivisions);
    }
    
    // Render model if loaded
    if (model && model->IsLoaded()) {
        renderer.Render(*model, camera);
    }
    
    EndMode3D();
    
    EndTextureMode();
}

} // namespace AAV

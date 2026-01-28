#pragma once

#include "model/Model.h"
#include "model/ModelLoader.h"
#include "ui/MainMenuBar.h"
#include "ui/SceneWindow.h"
#include "ui/InspectorWindow.h"
#include <memory>

namespace AAV {

class Application {
public:
    Application();
    ~Application();
    
    bool Initialize(int width, int height, const char* title);
    void Run();
    void Shutdown();
    
private:
    void SetupImGuiDocking();
    void HandleCallbacks();
    void OpenModelDialog();
    void LoadModelFile(const std::string& filePath);
    
    // UI Windows
    MainMenuBar menuBar;
    SceneWindow sceneWindow;
    InspectorWindow inspectorWindow;
    
    // Model data
    std::shared_ptr<Model> currentModel;
    ModelLoader modelLoader;
    
    // Application state
    bool initialized;
    bool showImGuiDemo;
    int screenWidth;
    int screenHeight;
};

} // namespace AAV

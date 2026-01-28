#pragma once

#include "model/Model.h"
#include "model/ModelLoader.h"
#include "ui/MainMenuBar.h"
#include "ui/SceneWindow.h"
#include "ui/InspectorWindow.h"
#include "ui/HierarchyWindow.h"
#include "ui/LogWindow.h"
#include "ui/ModelInfoWindow.h"
#include "ui/LightWindow.h"
#include "ui/SettingsWindow.h"
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
    void SetupDefaultLayout();
    void LoadLayoutPreset(int presetIndex);
    void HandleCallbacks();
    void OpenModelDialog();
    void LoadModelFile(const std::string& filePath);
    
    // UI Windows
    MainMenuBar menuBar;
    SceneWindow sceneWindow;
    InspectorWindow inspectorWindow;
    HierarchyWindow hierarchyWindow;
    LogWindow logWindow;
    ModelInfoWindow modelInfoWindow;
    LightWindow lightWindow;
    SettingsWindow settingsWindow;
    
    // Model data
    std::shared_ptr<Model> currentModel;
    ModelLoader modelLoader;
    NodeData* selectedNode;  // Currently selected node in hierarchy
    
    // Application state
    bool initialized;
    bool showImGuiDemo;
    int screenWidth;
    int screenHeight;
};

} // namespace AAV

#pragma once

#include <functional>
#include <string>

namespace AAV {

class MainMenuBar {
public:
    MainMenuBar();
    ~MainMenuBar();
    
    void Render();
    
    // Callbacks
    std::function<void()> onOpenFile;
    std::function<void()> onExit;
    std::function<void()> onToggleInspector;
    std::function<void()> onToggleScene;
    std::function<void()> onToggleHierarchy;
    std::function<void()> onToggleLog;
    std::function<void()> onToggleModelInfo;
    std::function<void()> onResetCamera;
    std::function<void(int)> onLoadLayout;  // 0=Classic, 1=Wide Inspector, 2=Full Scene
    std::function<void()> onAbout;
    std::function<void()> onShowControls;
    std::function<void()> onToggleImGuiDemo;
    
    void SetInspectorVisible(bool visible) { inspectorVisible = visible; }
    void SetSceneVisible(bool visible) { sceneVisible = visible; }
    void SetHierarchyVisible(bool visible) { hierarchyVisible = visible; }
    void SetLogVisible(bool visible) { logVisible = visible; }
    void SetModelInfoVisible(bool visible) { modelInfoVisible = visible; }
    
private:
    bool inspectorVisible;
    bool sceneVisible;
    bool hierarchyVisible;
    bool logVisible;
    bool modelInfoVisible;
    bool showAboutPopup;
    bool showControlsPopup;
};

} // namespace AAV

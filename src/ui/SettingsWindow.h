#pragma once

#include "SceneWindow.h"
#include <memory>

namespace AAV {

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();
    
    void Render(SceneWindow* sceneWindow);
    
    void Open() { showModal = true; }
    
private:
    bool showModal;
};

} // namespace AAV

#pragma once

#include "SceneWindow.h"
#include <memory>

namespace AAV {

class LightWindow {
public:
    LightWindow();
    ~LightWindow();
    
    void Render(SceneWindow* sceneWindow);
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
    
private:
    bool visible;
};

} // namespace AAV

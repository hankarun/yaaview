#pragma once

#include "../model/Model.h"
#include <memory>

namespace AAV {

class ModelInfoWindow {
public:
    ModelInfoWindow();
    ~ModelInfoWindow();
    
    void Render(std::shared_ptr<Model> model);
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
    
private:
    bool visible;
};

} // namespace AAV

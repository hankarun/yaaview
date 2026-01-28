#pragma once

#include "../util/Logger.h"

namespace AAV {

class LogWindow {
public:
    LogWindow();
    ~LogWindow();
    
    void Render();
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
    
private:
    bool visible;
    bool autoScroll;
    bool filterInfo;
    bool filterWarning;
    bool filterError;
    char searchBuffer[256];
    
    bool PassesFilter(const Logger::Entry& entry) const;
};

} // namespace AAV

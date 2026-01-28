#include "LogWindow.h"
#include "imgui.h"
#include <cstring>

namespace AAV {

LogWindow::LogWindow()
    : visible(true)
    , autoScroll(true)
    , filterInfo(true)
    , filterWarning(true)
    , filterError(true)
{
    memset(searchBuffer, 0, sizeof(searchBuffer));
}

LogWindow::~LogWindow() {
}

void LogWindow::Render() {
    if (!visible) return;
    
    ImGui::Begin("Log", &visible);
    
    // Toolbar
    if (ImGui::Button("Clear")) {
        Logger::Clear();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);
    
    ImGui::SameLine();
    ImGui::Checkbox("Info", &filterInfo);
    
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &filterWarning);
    
    ImGui::SameLine();
    ImGui::Checkbox("Error", &filterError);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Filter messages by text");
    }
    
    ImGui::Separator();
    
    // Log display area
    ImGui::BeginChild("LogScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    const auto& entries = Logger::GetEntries();
    
    for (const auto& entry : entries) {
        if (!PassesFilter(entry)) {
            continue;
        }
        
        // Color code by level
        ImVec4 color;
        const char* levelStr;
        
        switch (entry.level) {
            case Logger::Level::Info:
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White
                levelStr = "[INFO]";
                break;
            case Logger::Level::Warning:
                color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
                levelStr = "[WARN]";
                break;
            case Logger::Level::Error:
                color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);  // Red
                levelStr = "[ERROR]";
                break;
        }
        
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", entry.timestamp.c_str());
        ImGui::SameLine();
        ImGui::TextColored(color, "%s", levelStr);
        ImGui::SameLine();
        ImGui::TextColored(color, "%s", entry.message.c_str());
    }
    
    // Auto-scroll to bottom
    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    ImGui::End();
}

bool LogWindow::PassesFilter(const Logger::Entry& entry) const {
    // Check level filter
    bool levelMatch = false;
    switch (entry.level) {
        case Logger::Level::Info:
            levelMatch = filterInfo;
            break;
        case Logger::Level::Warning:
            levelMatch = filterWarning;
            break;
        case Logger::Level::Error:
            levelMatch = filterError;
            break;
    }
    
    if (!levelMatch) {
        return false;
    }
    
    // Check text filter
    if (searchBuffer[0] != '\0') {
        std::string lowerMessage = entry.message;
        std::string lowerSearch = searchBuffer;
        
        // Simple case-insensitive search
        for (char& c : lowerMessage) c = std::tolower(c);
        for (char& c : lowerSearch) c = std::tolower(c);
        
        if (lowerMessage.find(lowerSearch) == std::string::npos) {
            return false;
        }
    }
    
    return true;
}

} // namespace AAV

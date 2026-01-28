#pragma once

#include <string>
#include <vector>

namespace AAV {

class Logger {
public:
    enum class Level {
        Info,
        Warning,
        Error
    };
    
    struct Entry {
        std::string timestamp;
        Level level;
        std::string message;
    };
    
    // Static logging methods
    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    
    // Access log entries
    static const std::vector<Entry>& GetEntries();
    static void Clear();
    
private:
    static std::vector<Entry> entries;
    static std::string GetTimestamp();
    static void AddEntry(Level level, const std::string& message);
};

} // namespace AAV

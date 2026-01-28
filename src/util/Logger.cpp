#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace AAV {

// Initialize static member
std::vector<Logger::Entry> Logger::entries;

void Logger::Info(const std::string& message) {
    AddEntry(Level::Info, message);
}

void Logger::Warning(const std::string& message) {
    AddEntry(Level::Warning, message);
}

void Logger::Error(const std::string& message) {
    AddEntry(Level::Error, message);
}

const std::vector<Logger::Entry>& Logger::GetEntries() {
    return entries;
}

void Logger::Clear() {
    entries.clear();
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

void Logger::AddEntry(Level level, const std::string& message) {
    Entry entry;
    entry.timestamp = GetTimestamp();
    entry.level = level;
    entry.message = message;
    entries.push_back(entry);
}

} // namespace AAV

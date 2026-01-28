#pragma once

#include "Model.h"
#include <memory>
#include <string>

namespace AAV {

class ModelLoader {
public:
    ModelLoader();
    ~ModelLoader();
    
    // Load a model from file
    std::shared_ptr<Model> LoadModel(const std::string& filePath);
    
    // Get last error message
    std::string GetLastError() const { return lastError; }
    
private:
    std::string lastError;
};

} // namespace AAV

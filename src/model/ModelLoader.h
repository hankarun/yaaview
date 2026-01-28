#pragma once

#include "Model.h"
#include <memory>
#include <string>

// Forward declarations for Assimp types
struct aiNode;
struct aiMaterial;
struct aiTexture;

// Include for aiTextureType enum
#include <assimp/material.h>

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
    std::string modelDirectory;  // Store model's directory for texture loading
    
    // Recursive node processing
    std::shared_ptr<NodeData> ProcessNode(aiNode* aiNode, NodeData* parent);
    
    // Texture loading helpers
    Texture2D LoadTextureFromFile(const std::string& path);
    Texture2D LoadEmbeddedTexture(const aiTexture* aiTex);
    std::string ExtractTexturePath(aiMaterial* material, aiTextureType type);
};

} // namespace AAV

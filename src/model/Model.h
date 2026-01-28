#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <memory>

namespace AAV {

struct MeshData {
    std::string name;
    Mesh mesh;
    Material material;
    int materialIndex;
    Vector3 minBounds;
    Vector3 maxBounds;
};

struct MaterialData {
    std::string name;
    Color diffuseColor;
    Color specularColor;
    Color ambientColor;
    float shininess;
    std::string diffuseTexturePath;
    std::string specularTexturePath;
    std::string normalTexturePath;
};

struct AnimationData {
    std::string name;
    double duration;
    double ticksPerSecond;
};

struct NodeData {
    std::string name;
    Matrix transform;
    std::vector<int> meshIndices;
    std::vector<std::shared_ptr<NodeData>> children;
    NodeData* parent;
    
    NodeData() : parent(nullptr) {}
};

class Model {
public:
    Model();
    ~Model();
    
    // Model info
    std::string GetFilename() const { return filename; }
    std::string GetFilePath() const { return filePath; }
    int GetMeshCount() const { return static_cast<int>(meshes.size()); }
    int GetMaterialCount() const { return static_cast<int>(materials.size()); }
    int GetAnimationCount() const { return static_cast<int>(animations.size()); }
    
    // Statistics
    int GetTotalVertices() const;
    int GetTotalFaces() const;
    Vector3 GetBoundingBoxMin() const { return boundingBoxMin; }
    Vector3 GetBoundingBoxMax() const { return boundingBoxMax; }
    Vector3 GetBoundingBoxSize() const;
    Vector3 GetCenter() const;
    
    // Transform
    Vector3 GetPosition() const { return position; }
    Vector3 GetRotation() const { return rotation; }
    Vector3 GetScale() const { return scale; }
    
    void SetPosition(const Vector3& pos) { position = pos; UpdateTransform(); }
    void SetRotation(const Vector3& rot) { rotation = rot; UpdateTransform(); }
    void SetScale(const Vector3& scl) { scale = scl; UpdateTransform(); }
    void ResetTransform();
    
    Matrix GetTransformMatrix() const { return transformMatrix; }
    
    // Data access
    const std::vector<MeshData>& GetMeshes() const { return meshes; }
    const std::vector<MaterialData>& GetMaterials() const { return materials; }
    const std::vector<AnimationData>& GetAnimations() const { return animations; }
    std::shared_ptr<NodeData> GetRootNode() const { return rootNode; }
    
    // Data setters (used by ModelLoader)
    void SetFilename(const std::string& name) { filename = name; }
    void SetFilePath(const std::string& path) { filePath = path; }
    void AddMesh(const MeshData& mesh);
    void AddMaterial(const MaterialData& material) { materials.push_back(material); }
    void AddAnimation(const AnimationData& animation) { animations.push_back(animation); }
    void SetRootNode(std::shared_ptr<NodeData> node) { rootNode = node; }
    void CalculateBoundingBox();
    
    // State
    bool IsLoaded() const { return loaded; }
    void SetLoaded(bool state) { loaded = state; }
    
private:
    void UpdateTransform();
    
    std::string filename;
    std::string filePath;
    
    std::vector<MeshData> meshes;
    std::vector<MaterialData> materials;
    std::vector<AnimationData> animations;
    std::shared_ptr<NodeData> rootNode;
    
    Vector3 position;
    Vector3 rotation;  // Euler angles in degrees
    Vector3 scale;
    Matrix transformMatrix;
    
    Vector3 boundingBoxMin;
    Vector3 boundingBoxMax;
    
    bool loaded;
};

} // namespace AAV

#include "Model.h"
#include <cmath>
#include <limits>

namespace AAV {

Model::Model() 
    : position({0.0f, 0.0f, 0.0f})
    , rotation({0.0f, 0.0f, 0.0f})
    , scale({1.0f, 1.0f, 1.0f})
    , transformMatrix(MatrixIdentity())
    , boundingBoxMin({0.0f, 0.0f, 0.0f})
    , boundingBoxMax({0.0f, 0.0f, 0.0f})
    , loaded(false)
{
}

Model::~Model() {
    // Cleanup meshes
    for (auto& meshData : meshes) {
        UnloadMesh(meshData.mesh);
        UnloadMaterial(meshData.material);
    }
    
    // Cleanup textures from MaterialData
    for (auto& mat : materials) {
        if (mat.hasDiffuseTexture && mat.diffuseTexture.id != 0) {
            UnloadTexture(mat.diffuseTexture);
        }
        if (mat.hasSpecularTexture && mat.specularTexture.id != 0) {
            UnloadTexture(mat.specularTexture);
        }
        if (mat.hasNormalTexture && mat.normalTexture.id != 0) {
            UnloadTexture(mat.normalTexture);
        }
        if (mat.hasMetalnessTexture && mat.metalnessTexture.id != 0) {
            UnloadTexture(mat.metalnessTexture);
        }
        if (mat.hasRoughnessTexture && mat.roughnessTexture.id != 0) {
            UnloadTexture(mat.roughnessTexture);
        }
        if (mat.hasAOTexture && mat.aoTexture.id != 0) {
            UnloadTexture(mat.aoTexture);
        }
        if (mat.hasEmissiveTexture && mat.emissiveTexture.id != 0) {
            UnloadTexture(mat.emissiveTexture);
        }
    }
}

int Model::GetTotalVertices() const {
    int total = 0;
    for (const auto& meshData : meshes) {
        total += meshData.mesh.vertexCount;
    }
    return total;
}

int Model::GetTotalFaces() const {
    int total = 0;
    for (const auto& meshData : meshes) {
        total += meshData.mesh.triangleCount;
    }
    return total;
}

int Model::GetTotalTexturesLoaded() const {
    int count = 0;
    for (const auto& mat : materials) {
        if (mat.hasDiffuseTexture) count++;
        if (mat.hasSpecularTexture) count++;
        if (mat.hasNormalTexture) count++;
        if (mat.hasMetalnessTexture) count++;
        if (mat.hasRoughnessTexture) count++;
        if (mat.hasAOTexture) count++;
        if (mat.hasEmissiveTexture) count++;
    }
    return count;
}

Vector3 Model::GetBoundingBoxSize() const {
    return Vector3Subtract(boundingBoxMax, boundingBoxMin);
}

Vector3 Model::GetCenter() const {
    return Vector3Scale(Vector3Add(boundingBoxMin, boundingBoxMax), 0.5f);
}

void Model::ResetTransform() {
    position = {0.0f, 0.0f, 0.0f};
    rotation = {0.0f, 0.0f, 0.0f};
    scale = {1.0f, 1.0f, 1.0f};
    UpdateTransform();
}

void Model::AddMesh(const MeshData& mesh) {
    meshes.push_back(mesh);
}

void Model::CalculateBoundingBox() {
    if (meshes.empty()) {
        boundingBoxMin = {0.0f, 0.0f, 0.0f};
        boundingBoxMax = {0.0f, 0.0f, 0.0f};
        return;
    }
    
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();
    
    for (const auto& meshData : meshes) {
        if (meshData.minBounds.x < minX) minX = meshData.minBounds.x;
        if (meshData.minBounds.y < minY) minY = meshData.minBounds.y;
        if (meshData.minBounds.z < minZ) minZ = meshData.minBounds.z;
        if (meshData.maxBounds.x > maxX) maxX = meshData.maxBounds.x;
        if (meshData.maxBounds.y > maxY) maxY = meshData.maxBounds.y;
        if (meshData.maxBounds.z > maxZ) maxZ = meshData.maxBounds.z;
    }
    
    boundingBoxMin = {minX, minY, minZ};
    boundingBoxMax = {maxX, maxY, maxZ};
}

void Model::UpdateTransform() {
    // Build transform matrix: Scale * Rotation * Translation
    Matrix matScale = MatrixScale(scale.x, scale.y, scale.z);
    Matrix matRotation = MatrixRotateXYZ({
        rotation.x * DEG2RAD,
        rotation.y * DEG2RAD,
        rotation.z * DEG2RAD
    });
    Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);
    
    transformMatrix = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
}

} // namespace AAV

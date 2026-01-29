#include "ModelLoader.h"
#include "../util/Logger.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/texture.h>
#include <iostream>

namespace AAV {

// Forward declaration
static int CountNodes(NodeData* node);

ModelLoader::ModelLoader() {
}

ModelLoader::~ModelLoader() {
}

std::shared_ptr<Model> ModelLoader::LoadModel(const std::string& filePath) {
    lastError = "";
    
    // Create Assimp importer
    Assimp::Importer importer;
    
    // Load the scene with useful postprocessing flags
    // Raylib uses Y-up right-handed coordinate system
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |           // Convert all primitives to triangles
        aiProcess_GenNormals |            // Generate normals if not present
        aiProcess_CalcTangentSpace |      // Calculate tangent space for normal mapping
        aiProcess_JoinIdenticalVertices | // Optimize by joining identical vertices
        aiProcess_SortByPType |           // Sort by primitive type
        aiProcess_FlipUVs |               // Flip UVs for OpenGL
        aiProcess_ConvertToLeftHanded     // Convert to left-handed (Raylib Y-up)
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        lastError = "Failed to load model: " + std::string(importer.GetErrorString());
        return nullptr;
    }
    
    // Create model
    auto model = std::make_shared<Model>();
    model->SetFilePath(filePath);
    
    // Extract filename from path
    size_t lastSlash = filePath.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos) ? 
        filePath.substr(lastSlash + 1) : filePath;
    model->SetFilename(filename);
    
    // Store model directory for texture path resolution
    modelDirectory = (lastSlash != std::string::npos) ? 
        filePath.substr(0, lastSlash) : ".";
    
    Logger::Info("Processing materials and textures...");
    
    // Load materials
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* aiMat = scene->mMaterials[i];
        MaterialData material;
        
        aiString name;
        aiMat->Get(AI_MATKEY_NAME, name);
        material.name = name.C_Str();
        
        // Get colors
        aiColor3D diffuse(1.0f, 1.0f, 1.0f);
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        material.diffuseColor = {
            (unsigned char)(diffuse.r * 255),
            (unsigned char)(diffuse.g * 255),
            (unsigned char)(diffuse.b * 255),
            255
        };
        
        aiColor3D specular(1.0f, 1.0f, 1.0f);
        aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        material.specularColor = {
            (unsigned char)(specular.r * 255),
            (unsigned char)(specular.g * 255),
            (unsigned char)(specular.b * 255),
            255
        };
        
        aiColor3D ambient(0.2f, 0.2f, 0.2f);
        aiMat->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        material.ambientColor = {
            (unsigned char)(ambient.r * 255),
            (unsigned char)(ambient.g * 255),
            (unsigned char)(ambient.b * 255),
            255
        };
        
        float shininess = 32.0f;
        aiMat->Get(AI_MATKEY_SHININESS, shininess);
        material.shininess = shininess;
        
        // Get PBR properties (GLTF uses these)
        float metallicFactor = 0.0f;
        aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor);
        material.metallicFactor = metallicFactor;
        
        float roughnessFactor = 0.5f;
        aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor);
        material.roughnessFactor = roughnessFactor;
        
        // === TEXTURE LOADING ===
        
        // Helper lambda to load a texture (external or embedded)
        auto loadTexture = [&](aiTextureType type) -> Texture2D {
            std::string texPath = ExtractTexturePath(aiMat, type);
            if (texPath.empty()) {
                return {0};
            }
            
            // Check if embedded (format: "*0", "*1", "*2", etc.)
            if (texPath[0] == '*') {
                int texIndex = std::atoi(texPath.c_str() + 1);
                if (texIndex >= 0 && texIndex < (int)scene->mNumTextures) {
                    return LoadEmbeddedTexture(scene->mTextures[texIndex]);
                } else {
                    Logger::Error("Invalid embedded texture index: " + texPath);
                    return {0};
                }
            } else {
                // External file
                return LoadTextureFromFile(texPath);
            }
        };
        
        // Load diffuse texture
        material.diffuseTexturePath = ExtractTexturePath(aiMat, aiTextureType_DIFFUSE);
        if (!material.diffuseTexturePath.empty()) {
            material.diffuseTexture = loadTexture(aiTextureType_DIFFUSE);
            material.hasDiffuseTexture = (material.diffuseTexture.id != 0);
        }
        
        // Load specular texture
        material.specularTexturePath = ExtractTexturePath(aiMat, aiTextureType_SPECULAR);
        if (!material.specularTexturePath.empty()) {
            material.specularTexture = loadTexture(aiTextureType_SPECULAR);
            material.hasSpecularTexture = (material.specularTexture.id != 0);
        }
        
        // Load normal map
        material.normalTexturePath = ExtractTexturePath(aiMat, aiTextureType_NORMALS);
        if (material.normalTexturePath.empty()) {
            // Also try HEIGHT map as normal map (some exporters use this)
            material.normalTexturePath = ExtractTexturePath(aiMat, aiTextureType_HEIGHT);
        }
        if (!material.normalTexturePath.empty()) {
            aiTextureType normalType = aiTextureType_NORMALS;
            material.normalTexture = loadTexture(normalType);
            if (material.normalTexture.id == 0) {
                normalType = aiTextureType_HEIGHT;
                material.normalTexture = loadTexture(normalType);
            }
            material.hasNormalTexture = (material.normalTexture.id != 0);
        }
        
        // Load metalness texture (PBR)
        material.metalnessTexturePath = ExtractTexturePath(aiMat, aiTextureType_METALNESS);
        if (!material.metalnessTexturePath.empty()) {
            material.metalnessTexture = loadTexture(aiTextureType_METALNESS);
            material.hasMetalnessTexture = (material.metalnessTexture.id != 0);
        }
        
        // Load roughness texture (PBR)
        material.roughnessTexturePath = ExtractTexturePath(aiMat, aiTextureType_DIFFUSE_ROUGHNESS);
        if (!material.roughnessTexturePath.empty()) {
            material.roughnessTexture = loadTexture(aiTextureType_DIFFUSE_ROUGHNESS);
            material.hasRoughnessTexture = (material.roughnessTexture.id != 0);
        }
        
        // Load ambient occlusion texture
        material.aoTexturePath = ExtractTexturePath(aiMat, aiTextureType_AMBIENT_OCCLUSION);
        if (material.aoTexturePath.empty()) {
            // Also try LIGHTMAP as AO (some exporters use this)
            material.aoTexturePath = ExtractTexturePath(aiMat, aiTextureType_LIGHTMAP);
        }
        if (!material.aoTexturePath.empty()) {
            aiTextureType aoType = aiTextureType_AMBIENT_OCCLUSION;
            material.aoTexture = loadTexture(aoType);
            if (material.aoTexture.id == 0) {
                aoType = aiTextureType_LIGHTMAP;
                material.aoTexture = loadTexture(aoType);
            }
            material.hasAOTexture = (material.aoTexture.id != 0);
        }
        
        // Load emissive texture
        material.emissiveTexturePath = ExtractTexturePath(aiMat, aiTextureType_EMISSIVE);
        if (!material.emissiveTexturePath.empty()) {
            material.emissiveTexture = loadTexture(aiTextureType_EMISSIVE);
            material.hasEmissiveTexture = (material.emissiveTexture.id != 0);
        }
        
        model->AddMaterial(material);
    }
    
    Logger::Info("Loaded " + std::to_string(scene->mNumMaterials) + " materials");
    Logger::Info("Processing meshes...");
    
    // Load meshes
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* aiMesh = scene->mMeshes[i];
        
        MeshData meshData;
        meshData.name = aiMesh->mName.C_Str();
        meshData.materialIndex = aiMesh->mMaterialIndex;
        
        // Allocate mesh data
        Mesh mesh = {0};
        mesh.vertexCount = aiMesh->mNumVertices;
        mesh.triangleCount = aiMesh->mNumFaces;
        
        // Allocate vertices
        mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
        mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
        if (aiMesh->mTextureCoords[0]) {
            mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
        }
        if (aiMesh->mTangents) {
            mesh.tangents = (float*)MemAlloc(mesh.vertexCount * 4 * sizeof(float));
        }
        
        // Calculate bounding box
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();
        float maxZ = std::numeric_limits<float>::lowest();
        
        // Copy vertex data
        for (unsigned int j = 0; j < mesh.vertexCount; j++) {
            mesh.vertices[j * 3 + 0] = aiMesh->mVertices[j].x;
            mesh.vertices[j * 3 + 1] = aiMesh->mVertices[j].y;
            mesh.vertices[j * 3 + 2] = aiMesh->mVertices[j].z;
            
            mesh.normals[j * 3 + 0] = aiMesh->mNormals[j].x;
            mesh.normals[j * 3 + 1] = aiMesh->mNormals[j].y;
            mesh.normals[j * 3 + 2] = aiMesh->mNormals[j].z;
            
            if (aiMesh->mTextureCoords[0]) {
                mesh.texcoords[j * 2 + 0] = aiMesh->mTextureCoords[0][j].x;
                mesh.texcoords[j * 2 + 1] = aiMesh->mTextureCoords[0][j].y;
            }
            
            // Copy tangent data for PBR normal mapping
            if (aiMesh->mTangents) {
                mesh.tangents[j * 4 + 0] = aiMesh->mTangents[j].x;
                mesh.tangents[j * 4 + 1] = aiMesh->mTangents[j].y;
                mesh.tangents[j * 4 + 2] = aiMesh->mTangents[j].z;
                // Calculate handedness (w component) using bitangent
                // If bitangent exists, determine handedness via cross product
                if (aiMesh->mBitangents) {
                    // Compute cross product of normal and tangent
                    float crossX = aiMesh->mNormals[j].y * aiMesh->mTangents[j].z - aiMesh->mNormals[j].z * aiMesh->mTangents[j].y;
                    float crossY = aiMesh->mNormals[j].z * aiMesh->mTangents[j].x - aiMesh->mNormals[j].x * aiMesh->mTangents[j].z;
                    float crossZ = aiMesh->mNormals[j].x * aiMesh->mTangents[j].y - aiMesh->mNormals[j].y * aiMesh->mTangents[j].x;
                    
                    // Check if cross product aligns with bitangent
                    float dotProduct = crossX * aiMesh->mBitangents[j].x + 
                                     crossY * aiMesh->mBitangents[j].y + 
                                     crossZ * aiMesh->mBitangents[j].z;
                    mesh.tangents[j * 4 + 3] = (dotProduct < 0.0f) ? -1.0f : 1.0f;
                } else {
                    mesh.tangents[j * 4 + 3] = 1.0f;  // Default handedness
                }
            }
            
            // Update bounding box
            if (aiMesh->mVertices[j].x < minX) minX = aiMesh->mVertices[j].x;
            if (aiMesh->mVertices[j].y < minY) minY = aiMesh->mVertices[j].y;
            if (aiMesh->mVertices[j].z < minZ) minZ = aiMesh->mVertices[j].z;
            if (aiMesh->mVertices[j].x > maxX) maxX = aiMesh->mVertices[j].x;
            if (aiMesh->mVertices[j].y > maxY) maxY = aiMesh->mVertices[j].y;
            if (aiMesh->mVertices[j].z > maxZ) maxZ = aiMesh->mVertices[j].z;
        }
        
        meshData.minBounds = {minX, minY, minZ};
        meshData.maxBounds = {maxX, maxY, maxZ};
        
        // Copy indices
        mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));
        for (unsigned int j = 0; j < aiMesh->mNumFaces; j++) {
            aiFace face = aiMesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                mesh.indices[j * 3 + k] = face.mIndices[k];
            }
        }
        
        // Upload mesh to GPU
        UploadMesh(&mesh, false);
        
        // Create material with textures
        Material material = LoadMaterialDefault();
        if (aiMesh->mMaterialIndex < model->GetMaterialCount()) {
            const MaterialData& matData = model->GetMaterials()[aiMesh->mMaterialIndex];
            
            // Apply diffuse color
            material.maps[MATERIAL_MAP_DIFFUSE].color = matData.diffuseColor;
            
            // Apply diffuse texture if loaded
            if (matData.hasDiffuseTexture) {
                material.maps[MATERIAL_MAP_DIFFUSE].texture = matData.diffuseTexture;
            }
            
            // Apply normal map if loaded
            if (matData.hasNormalTexture) {
                material.maps[MATERIAL_MAP_NORMAL].texture = matData.normalTexture;
            }
            
            // Apply specular map if loaded
            if (matData.hasSpecularTexture) {
                material.maps[MATERIAL_MAP_SPECULAR].texture = matData.specularTexture;
            }
            
            // Apply metalness map if loaded
            if (matData.hasMetalnessTexture) {
                material.maps[MATERIAL_MAP_METALNESS].texture = matData.metalnessTexture;
            }
            
            // Apply roughness map if loaded
            if (matData.hasRoughnessTexture) {
                material.maps[MATERIAL_MAP_ROUGHNESS].texture = matData.roughnessTexture;
            }
            
            // Apply AO map if loaded
            if (matData.hasAOTexture) {
                material.maps[MATERIAL_MAP_OCCLUSION].texture = matData.aoTexture;
            }
            
            // Apply emissive map if loaded
            if (matData.hasEmissiveTexture) {
                material.maps[MATERIAL_MAP_EMISSION].texture = matData.emissiveTexture;
            }
        }
        
        meshData.mesh = mesh;
        meshData.material = material;
        
        model->AddMesh(meshData);
    }
    
    Logger::Info("Loaded " + std::to_string(scene->mNumMeshes) + " meshes");
    Logger::Info("Processing animations...");
    
    // Load animations
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];
        AnimationData animData;
        animData.name = anim->mName.C_Str();
        animData.duration = anim->mDuration;
        animData.ticksPerSecond = anim->mTicksPerSecond;
        model->AddAnimation(animData);
    }
    
    Logger::Info("Loaded " + std::to_string(scene->mNumAnimations) + " animations");
    Logger::Info("Building node hierarchy...");
    
    // Build complete scene hierarchy recursively
    auto rootNode = ProcessNode(scene->mRootNode, nullptr);
    model->SetRootNode(rootNode);
    
    int totalNodes = CountNodes(rootNode.get());
    Logger::Info("Processed " + std::to_string(totalNodes) + " nodes");
    
    // Calculate overall bounding box
    model->CalculateBoundingBox();
    model->SetLoaded(true);
    
    // Log statistics
    Logger::Info("========================================");
    Logger::Info("Model loaded successfully: " + filename);
    Logger::Info("  Meshes: " + std::to_string(model->GetMeshCount()));
    Logger::Info("  Vertices: " + std::to_string(model->GetTotalVertices()));
    Logger::Info("  Faces: " + std::to_string(model->GetTotalFaces()));
    Logger::Info("  Materials: " + std::to_string(model->GetMaterialCount()));
    Logger::Info("  Textures: " + std::to_string(model->GetTotalTexturesLoaded()));
    Logger::Info("  Nodes: " + std::to_string(totalNodes));
    Logger::Info("  Animations: " + std::to_string(model->GetAnimationCount()));
    Logger::Info("========================================");
    
    return model;
}

// Helper function to count nodes recursively
static int CountNodes(NodeData* node) {
    if (!node) return 0;
    int count = 1;
    for (auto& child : node->children) {
        count += CountNodes(child.get());
    }
    return count;
}

std::shared_ptr<NodeData> ModelLoader::ProcessNode(aiNode* aiNode, NodeData* parent) {
    auto node = std::make_shared<NodeData>();
    
    // Copy node name
    node->name = aiNode->mName.C_Str();
    
    // Copy transform matrix from aiMatrix4x4 to raylib Matrix
    aiMatrix4x4& t = aiNode->mTransformation;
    node->transform = Matrix {
        t.a1, t.b1, t.c1, t.d1,  // Row 0
        t.a2, t.b2, t.c2, t.d2,  // Row 1
        t.a3, t.b3, t.c3, t.d3,  // Row 2
        t.a4, t.b4, t.c4, t.d4   // Row 3
    };
    
    // Set parent relationship
    node->parent = parent;
    
    // Copy mesh indices attached to this node
    for (unsigned int i = 0; i < aiNode->mNumMeshes; i++) {
        node->meshIndices.push_back(aiNode->mMeshes[i]);
    }
    
    // Recursively process all children
    node->children.reserve(aiNode->mNumChildren);
    for (unsigned int i = 0; i < aiNode->mNumChildren; i++) {
        auto childNode = ProcessNode(aiNode->mChildren[i], node.get());
        node->children.push_back(childNode);
    }
    
    return node;
}

std::string ModelLoader::ExtractTexturePath(aiMaterial* material, aiTextureType type) {
    if (material->GetTextureCount(type) > 0) {
        aiString path;
        material->GetTexture(type, 0, &path);
        return std::string(path.C_Str());
    }
    return "";
}

Texture2D ModelLoader::LoadTextureFromFile(const std::string& path) {
    Texture2D tex = {0};
    
    // Strategy 1: Try absolute path
    if (FileExists(path.c_str())) {
        tex = LoadTexture(path.c_str());
        if (tex.id != 0) {
            Logger::Info("  ✓ Loaded texture: " + path);
            return tex;
        }
    }
    
    // Strategy 2: Try relative to model directory
    std::string fullPath = modelDirectory + "/" + path;
    if (FileExists(fullPath.c_str())) {
        tex = LoadTexture(fullPath.c_str());
        if (tex.id != 0) {
            Logger::Info("  ✓ Loaded texture: " + fullPath);
            return tex;
        }
    }
    
    // Strategy 3: Try just filename (strip subdirectories)
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string filename = path.substr(lastSlash + 1);
        fullPath = modelDirectory + "/" + filename;
        if (FileExists(fullPath.c_str())) {
            tex = LoadTexture(fullPath.c_str());
            if (tex.id != 0) {
                Logger::Info("  ✓ Loaded texture: " + fullPath);
                return tex;
            }
        }
    }
    
    // Strategy 4: Try common texture folders
    const char* textureFolders[] = {"textures", "Textures", "tex", "images", "Images"};
    for (const char* folder : textureFolders) {
        fullPath = modelDirectory + "/" + folder + "/" + path;
        if (FileExists(fullPath.c_str())) {
            tex = LoadTexture(fullPath.c_str());
            if (tex.id != 0) {
                Logger::Info("  ✓ Loaded texture: " + fullPath);
                return tex;
            }
        }
        
        // Also try with just filename
        if (lastSlash != std::string::npos) {
            std::string filename = path.substr(lastSlash + 1);
            fullPath = modelDirectory + "/" + folder + "/" + filename;
            if (FileExists(fullPath.c_str())) {
                tex = LoadTexture(fullPath.c_str());
                if (tex.id != 0) {
                    Logger::Info("  ✓ Loaded texture: " + fullPath);
                    return tex;
                }
            }
        }
    }
    
    // All strategies failed
    Logger::Warning("  ✗ Failed to load texture: " + path);
    return tex;
}

Texture2D ModelLoader::LoadEmbeddedTexture(const aiTexture* aiTex) {
    if (!aiTex) {
        return {0};
    }
    
    Texture2D tex = {0};
    
    if (aiTex->mHeight == 0) {
        // Compressed format (JPEG, PNG, etc.)
        const char* formatHint = aiTex->achFormatHint;
        
        // Determine format for LoadImageFromMemory
        std::string format(formatHint);
        if (format.empty()) {
            format = "png";  // Default guess
        }
        
        // Load image from memory using raylib's stb_image wrapper
        Image img = LoadImageFromMemory(
            ("." + format).c_str(),           // Format hint (needs dot prefix)
            (unsigned char*)aiTex->pcData,    // Compressed data
            aiTex->mWidth                      // Size in bytes
        );
        
        if (img.data) {
            tex = LoadTextureFromImage(img);
            UnloadImage(img);
            Logger::Info("  ✓ Loaded embedded texture (" + format + ", " + 
                        std::to_string(tex.width) + "x" + std::to_string(tex.height) + ")");
        } else {
            Logger::Error("  ✗ Failed to decode embedded texture");
        }
    } else {
        // Uncompressed ARGB8888 format
        Image img = {
            aiTex->pcData,
            (int)aiTex->mWidth,
            (int)aiTex->mHeight,
            1,
            PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        
        // Need to copy data because LoadTextureFromImage expects ownership
        Image imgCopy = ImageCopy(img);
        tex = LoadTextureFromImage(imgCopy);
        UnloadImage(imgCopy);
        
        Logger::Info("  ✓ Loaded embedded texture (raw, " + 
                    std::to_string(tex.width) + "x" + std::to_string(tex.height) + ")");
    }
    
    return tex;
}

} // namespace AAV

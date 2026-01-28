#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

namespace AAV {

ModelLoader::ModelLoader() {
}

ModelLoader::~ModelLoader() {
}

std::shared_ptr<Model> ModelLoader::LoadModel(const std::string& filePath) {
    lastError = "";
    
    // Create Assimp importer
    Assimp::Importer importer;
    
    // Load the scene with useful postprocessing flags
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |           // Convert all primitives to triangles
        aiProcess_GenNormals |            // Generate normals if not present
        aiProcess_CalcTangentSpace |      // Calculate tangent space for normal mapping
        aiProcess_JoinIdenticalVertices | // Optimize by joining identical vertices
        aiProcess_SortByPType |           // Sort by primitive type
        aiProcess_FlipUVs                 // Flip UVs for OpenGL
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
        
        model->AddMaterial(material);
    }
    
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
        
        // Create material
        Material material = LoadMaterialDefault();
        if (aiMesh->mMaterialIndex < model->GetMaterialCount()) {
            const MaterialData& matData = model->GetMaterials()[aiMesh->mMaterialIndex];
            material.maps[MATERIAL_MAP_DIFFUSE].color = matData.diffuseColor;
        }
        
        meshData.mesh = mesh;
        meshData.material = material;
        
        model->AddMesh(meshData);
    }
    
    // Load animations
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];
        AnimationData animData;
        animData.name = anim->mName.C_Str();
        animData.duration = anim->mDuration;
        animData.ticksPerSecond = anim->mTicksPerSecond;
        model->AddAnimation(animData);
    }
    
    // Build scene hierarchy (simplified - just root node for now)
    auto rootNode = std::make_shared<NodeData>();
    rootNode->name = scene->mRootNode->mName.C_Str();
    model->SetRootNode(rootNode);
    
    // Calculate overall bounding box
    model->CalculateBoundingBox();
    model->SetLoaded(true);
    
    std::cout << "Model loaded successfully: " << filename << std::endl;
    std::cout << "  Meshes: " << model->GetMeshCount() << std::endl;
    std::cout << "  Vertices: " << model->GetTotalVertices() << std::endl;
    std::cout << "  Faces: " << model->GetTotalFaces() << std::endl;
    std::cout << "  Materials: " << model->GetMaterialCount() << std::endl;
    std::cout << "  Animations: " << model->GetAnimationCount() << std::endl;
    
    return model;
}

} // namespace AAV

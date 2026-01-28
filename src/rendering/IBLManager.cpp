#include "IBLManager.h"
#include "rlgl.h"
#include "raymath.h"
#include <iostream>
#include <cmath>
#include <cstring>

namespace AAV {

IBLManager::IBLManager()
    : environmentLoaded(false)
    , mapsGenerated(false)
    , skyboxEnabled(true)
{
    // Initialize empty textures
    environmentMap = {0};
    irradianceMap = {0};
    prefilterMap = {0};
    brdfLUT = {0};
    skyboxMesh = {0};
    skyboxShader = {0};
}

IBLManager::~IBLManager() {
    Cleanup();
}

bool IBLManager::Initialize() {
    // Load skybox shader
    skyboxShader = LoadShader("shaders/skybox.vs", "shaders/skybox.fs");
    if (skyboxShader.id == 0) {
        std::cerr << "Failed to load skybox shader" << std::endl;
        return false;
    }
    
    // Initialize shader locations
    skyboxShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(skyboxShader, "mvp");
    
    // Create skybox cube mesh (large cube for skybox)
    skyboxMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    
    // Generate default environment (simple gradient)
    GenerateDefaultEnvironment();
    
    std::cout << "IBL Manager initialized successfully" << std::endl;
    return true;
}

void IBLManager::Cleanup() {
    if (environmentMap.id != 0) UnloadTexture(environmentMap);
    if (irradianceMap.id != 0) UnloadTexture(irradianceMap);
    if (prefilterMap.id != 0) UnloadTexture(prefilterMap);
    if (brdfLUT.id != 0) UnloadTexture(brdfLUT);
    if (skyboxMesh.vboId != nullptr) UnloadMesh(skyboxMesh);
    if (skyboxShader.id != 0) UnloadShader(skyboxShader);
    
    environmentLoaded = false;
    mapsGenerated = false;
}

void IBLManager::GenerateDefaultEnvironment() {
    // Load the 6 cubemap faces from disk
    const char* facePaths[6] = {
        "hdr/Storforsen3/posx.jpg",  // Right  (+X)
        "hdr/Storforsen3/negx.jpg",  // Left   (-X)
        "hdr/Storforsen3/posy.jpg",  // Top    (+Y)
        "hdr/Storforsen3/negy.jpg",  // Bottom (-Y)
        "hdr/Storforsen3/posz.jpg",  // Front  (+Z)
        "hdr/Storforsen3/negz.jpg"   // Back   (-Z)
    };
    
    // Load all 6 face images
    Image faces[6];
    bool allLoaded = true;
    
    for (int i = 0; i < 6; i++) {
        faces[i] = LoadImage(facePaths[i]);
        if (faces[i].data == nullptr) {
            std::cerr << "Failed to load cubemap face: " << facePaths[i] << std::endl;
            allLoaded = false;
            break;
        }
    }
    
    if (!allLoaded) {
        // Clean up any loaded faces
        for (int i = 0; i < 6; i++) {
            if (faces[i].data != nullptr) {
                UnloadImage(faces[i]);
            }
        }
        
        // Fall back to simple blue gradient
        std::cout << "Using fallback gradient environment" << std::endl;
        GenerateFallbackEnvironment();
        return;
    }
    
    // Combine all 6 faces into a single contiguous buffer
    // rlLoadTextureCubemap expects: +X, -X, +Y, -Y, +Z, -Z (all face data concatenated)
    int faceSize = faces[0].width * faces[0].height;
    int bytesPerPixel = 3; // RGB format
    int totalSize = faceSize * bytesPerPixel * 6;
    
    unsigned char* combinedData = (unsigned char*)malloc(totalSize);
    if (!combinedData) {
        std::cerr << "Failed to allocate memory for cubemap data" << std::endl;
        for (int i = 0; i < 6; i++) {
            UnloadImage(faces[i]);
        }
        GenerateFallbackEnvironment();
        return;
    }
    
    // Copy each face data into the combined buffer
    for (int i = 0; i < 6; i++) {
        memcpy(combinedData + (i * faceSize * bytesPerPixel), 
               faces[i].data, 
               faceSize * bytesPerPixel);
    }
    
    // Create cubemap from combined data
    environmentMap.id = rlLoadTextureCubemap(combinedData, faces[0].width, faces[0].format, 1);
    environmentMap.width = faces[0].width;
    environmentMap.height = faces[0].height;
    environmentMap.mipmaps = 1;
    environmentMap.format = faces[0].format;
    
    // Free combined data and unload face images
    free(combinedData);
    for (int i = 0; i < 6; i++) {
        UnloadImage(faces[i]);
    }
    
    if (environmentMap.id == 0) {
        std::cerr << "Failed to create cubemap texture" << std::endl;
        GenerateFallbackEnvironment();
        return;
    }
    
    environmentLoaded = true;
    
    // Generate IBL maps from loaded environment
    GenerateIrradianceMap();
    GeneratePrefilterMap();
    GenerateBRDFLUT();
    
    std::cout << "Environment loaded from HDR cubemap files" << std::endl;
}

void IBLManager::GenerateFallbackEnvironment() {
    // Create simple blue gradient as fallback
    const int size = 64;
    Image faces[6];
    
    for (int i = 0; i < 6; i++) {
        faces[i] = GenImageColor(size, size, SKYBLUE);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float factor = (float)y / (float)size;
                Color gradColor = {
                    (unsigned char)(135 * (1.0f - factor * 0.5f)),
                    (unsigned char)(206 * (1.0f - factor * 0.3f)),
                    (unsigned char)(235 * (1.0f - factor * 0.1f)),
                    255
                };
                ImageDrawPixel(&faces[i], x, y, gradColor);
            }
        }
    }
    
    // Combine all 6 faces into a single buffer
    int faceSize = size * size;
    int bytesPerPixel = 4; // RGBA format for generated images
    int totalSize = faceSize * bytesPerPixel * 6;
    
    unsigned char* combinedData = (unsigned char*)malloc(totalSize);
    if (!combinedData) {
        std::cerr << "Failed to allocate memory for fallback cubemap" << std::endl;
        for (int i = 0; i < 6; i++) {
            UnloadImage(faces[i]);
        }
        return;
    }
    
    // Copy each face data into the combined buffer
    for (int i = 0; i < 6; i++) {
        memcpy(combinedData + (i * faceSize * bytesPerPixel), 
               faces[i].data, 
               faceSize * bytesPerPixel);
    }
    
    environmentMap.id = rlLoadTextureCubemap(combinedData, size, faces[0].format, 1);
    environmentMap.width = size;
    environmentMap.height = size;
    environmentMap.mipmaps = 1;
    environmentMap.format = faces[0].format;
    
    free(combinedData);
    for (int i = 0; i < 6; i++) {
        UnloadImage(faces[i]);
    }
    
    environmentLoaded = true;
    
    GenerateIrradianceMap();
    GeneratePrefilterMap();
    GenerateBRDFLUT();
}

void IBLManager::GenerateIrradianceMap() {
    if (!environmentLoaded) return;
    
    // For now, create a simple blurred version
    // In a full implementation, this would convolve the environment map
    const int size = 32;
    
    Image faces[6];
    for (int i = 0; i < 6; i++) {
        faces[i] = GenImageColor(size, size, (Color){100, 150, 200, 255});
    }
    
    // Combine all 6 faces into a single buffer
    int faceSize = size * size;
    int bytesPerPixel = 4; // RGBA format
    int totalSize = faceSize * bytesPerPixel * 6;
    
    unsigned char* combinedData = (unsigned char*)malloc(totalSize);
    if (combinedData) {
        for (int i = 0; i < 6; i++) {
            memcpy(combinedData + (i * faceSize * bytesPerPixel), 
                   faces[i].data, 
                   faceSize * bytesPerPixel);
        }
        
        irradianceMap.id = rlLoadTextureCubemap(combinedData, size, faces[0].format, 1);
        irradianceMap.width = size;
        irradianceMap.height = size;
        irradianceMap.mipmaps = 1;
        irradianceMap.format = faces[0].format;
        
        free(combinedData);
    }
    
    for (int i = 0; i < 6; i++) {
        UnloadImage(faces[i]);
    }
    
    std::cout << "Irradiance map generated" << std::endl;
}

void IBLManager::GeneratePrefilterMap() {
    if (!environmentLoaded) return;
    
    // Create prefiltered environment map with mipmaps
    // For now, use simplified version
    const int size = 128;
    
    Image faces[6];
    for (int i = 0; i < 6; i++) {
        faces[i] = GenImageColor(size, size, (Color){135, 206, 235, 255});
    }
    
    // Combine all 6 faces into a single buffer
    int faceSize = size * size;
    int bytesPerPixel = 4; // RGBA format
    int totalSize = faceSize * bytesPerPixel * 6;
    
    unsigned char* combinedData = (unsigned char*)malloc(totalSize);
    if (combinedData) {
        for (int i = 0; i < 6; i++) {
            memcpy(combinedData + (i * faceSize * bytesPerPixel), 
                   faces[i].data, 
                   faceSize * bytesPerPixel);
        }
        
        prefilterMap.id = rlLoadTextureCubemap(combinedData, size, faces[0].format, 1);
        prefilterMap.width = size;
        prefilterMap.height = size;
        prefilterMap.mipmaps = 1;
        prefilterMap.format = faces[0].format;
        
        // Generate mipmaps for roughness levels
        rlEnableTextureCubemap(prefilterMap.id);
        GenTextureMipmaps(&prefilterMap);
        
        free(combinedData);
    }
    
    for (int i = 0; i < 6; i++) {
        UnloadImage(faces[i]);
    }
    
    std::cout << "Prefilter map generated" << std::endl;
}

void IBLManager::GenerateBRDFLUT() {
    // Generate BRDF integration lookup texture
    const int size = 512;
    
    Image brdfImage = GenImageColor(size, size, WHITE);
    
    // Simple BRDF LUT approximation
    // In a full implementation, this would integrate the BRDF equation
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float NdotV = (float)x / (float)size;
            float roughness = (float)y / (float)size;
            
            // Simple approximation of BRDF integration
            float scale = NdotV * (1.0f - roughness * 0.5f);
            float bias = roughness * 0.1f;
            
            Color lutColor = {
                (unsigned char)(scale * 255.0f),
                (unsigned char)(bias * 255.0f),
                0,
                255
            };
            ImageDrawPixel(&brdfImage, x, y, lutColor);
        }
    }
    
    brdfLUT = LoadTextureFromImage(brdfImage);
    UnloadImage(brdfImage);
    
    std::cout << "BRDF LUT generated" << std::endl;
    
    mapsGenerated = true;
}

bool IBLManager::LoadEnvironmentMap(const std::string& hdriPath) {
    // Try to load HDR image
    Image hdrImage = LoadImage(hdriPath.c_str());
    
    if (hdrImage.data == nullptr) {
        std::cerr << "Failed to load HDR image: " << hdriPath << std::endl;
        return false;
    }
    
    // Convert to cubemap
    environmentMap = LoadCubemapFromEquirectangular(hdriPath);
    
    UnloadImage(hdrImage);
    
    if (environmentMap.id == 0) {
        std::cerr << "Failed to convert HDR to cubemap" << std::endl;
        return false;
    }
    
    environmentLoaded = true;
    
    // Regenerate IBL maps
    GenerateIrradianceMap();
    GeneratePrefilterMap();
    GenerateBRDFLUT();
    
    std::cout << "Environment map loaded: " << hdriPath << std::endl;
    return true;
}

TextureCubemap IBLManager::LoadCubemapFromEquirectangular(const std::string& hdriPath) {
    // Load HDR texture
    Texture2D hdrTexture = LoadTexture(hdriPath.c_str());
    
    if (hdrTexture.id == 0) {
        return {0};
    }
    
    // For simplicity, create a basic cubemap
    // A full implementation would project equirectangular to cubemap faces
    const int size = 512;
    Image faces[6];
    
    for (int i = 0; i < 6; i++) {
        faces[i] = GenImageColor(size, size, SKYBLUE);
    }
    
    TextureCubemap cubemap = LoadTextureCubemap(faces[0], CUBEMAP_LAYOUT_AUTO_DETECT);
    
    for (int i = 0; i < 6; i++) {
        UnloadImage(faces[i]);
    }
    
    UnloadTexture(hdrTexture);
    
    return cubemap;
}

void IBLManager::RenderSkybox(Camera3D camera, int viewportWidth, int viewportHeight, float exposure) {
    if (!skyboxEnabled || !environmentLoaded || environmentMap.id == 0) {
        return;
    }
    
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    
    // Set uniforms
    int exposureLoc = GetShaderLocation(skyboxShader, "exposure");
    SetShaderValue(skyboxShader, exposureLoc, &exposure, SHADER_UNIFORM_FLOAT);
    
    // Calculate view matrix WITHOUT translation (skybox centered at camera)
    Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
    Matrix viewMatrix = MatrixLookAt(Vector3Zero(), cameraDir, camera.up);
    
    // Calculate projection matrix using RENDER TEXTURE aspect ratio
    float aspect = (float)viewportWidth / (float)viewportHeight;
    Matrix projMatrix = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.01, 1000.0);
    
    // Include scale in MVP calculation so it affects projection
    Matrix scaleMatrix = MatrixScale(500.0f, 500.0f, 500.0f);
    Matrix mvpMatrix = MatrixMultiply(MatrixMultiply(scaleMatrix, viewMatrix), projMatrix);
    
    SetShaderValueMatrix(skyboxShader, skyboxShader.locs[SHADER_LOC_MATRIX_MVP], mvpMatrix);
    
    // Create material with cubemap texture assigned
    Material skyboxMat = LoadMaterialDefault();
    skyboxMat.shader = skyboxShader;
    skyboxMat.maps[MATERIAL_MAP_CUBEMAP].texture = environmentMap;
    
    // Draw skybox without additional transform (already in MVP)
    DrawMesh(skyboxMesh, skyboxMat, MatrixIdentity());
    
    // Don't unload material - it would unload our shader
    // The material is lightweight and only allocated on stack
    
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

} // namespace AAV

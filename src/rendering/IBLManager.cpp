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
    skyboxShader.locs[SHADER_LOC_MAP_CUBEMAP] = GetShaderLocation(skyboxShader, "environmentMap");
    
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

// ============================================================================
// HELPER FUNCTIONS FOR IBL GENERATION
// ============================================================================

// Convert 3D direction to cubemap face index and UV coordinates
void IBLManager::DirectionToCubemapUV(Vector3 direction, int* faceIndex, float* u, float* v) {
    float absX = fabsf(direction.x);
    float absY = fabsf(direction.y);
    float absZ = fabsf(direction.z);
    
    float ma; // Major axis
    Vector2 uv;
    
    if (absX >= absY && absX >= absZ) {
        // X axis dominant
        if (direction.x > 0) {
            // +X face
            *faceIndex = 0;
            ma = absX;
            uv.x = -direction.z;
            uv.y = -direction.y;
        } else {
            // -X face
            *faceIndex = 1;
            ma = absX;
            uv.x = direction.z;
            uv.y = -direction.y;
        }
    } else if (absY >= absX && absY >= absZ) {
        // Y axis dominant
        if (direction.y > 0) {
            // +Y face
            *faceIndex = 2;
            ma = absY;
            uv.x = direction.x;
            uv.y = direction.z;
        } else {
            // -Y face
            *faceIndex = 3;
            ma = absY;
            uv.x = direction.x;
            uv.y = -direction.z;
        }
    } else {
        // Z axis dominant
        if (direction.z > 0) {
            // +Z face
            *faceIndex = 4;
            ma = absZ;
            uv.x = direction.x;
            uv.y = -direction.y;
        } else {
            // -Z face
            *faceIndex = 5;
            ma = absZ;
            uv.x = -direction.x;
            uv.y = -direction.y;
        }
    }
    
    // Convert to [0, 1] range
    *u = (uv.x / ma + 1.0f) * 0.5f;
    *v = (uv.y / ma + 1.0f) * 0.5f;
}

// Get 3D direction from cubemap face and UV coordinates
Vector3 IBLManager::GetCubemapDirection(int face, float u, float v) {
    // Convert from [0, 1] to [-1, 1]
    float uc = 2.0f * u - 1.0f;
    float vc = 2.0f * v - 1.0f;
    
    Vector3 dir;
    switch (face) {
        case 0: dir = {1.0f, -vc, -uc}; break;  // +X
        case 1: dir = {-1.0f, -vc, uc}; break;  // -X
        case 2: dir = {uc, 1.0f, vc}; break;    // +Y
        case 3: dir = {uc, -1.0f, -vc}; break;  // -Y
        case 4: dir = {uc, -vc, 1.0f}; break;   // +Z
        case 5: dir = {-uc, -vc, -1.0f}; break; // -Z
    }
    
    return Vector3Normalize(dir);
}

// Sample environment map using 3D direction
Color IBLManager::SampleEnvironmentMap(const Image faces[6], Vector3 direction) {
    int faceIndex;
    float u, v;
    DirectionToCubemapUV(direction, &faceIndex, &u, &v);
    
    // Clamp UV coordinates
    u = fmaxf(0.0f, fminf(1.0f, u));
    v = fmaxf(0.0f, fminf(1.0f, v));
    
    // Convert to pixel coordinates
    int x = (int)(u * (faces[faceIndex].width - 1));
    int y = (int)(v * (faces[faceIndex].height - 1));
    
    // Clamp pixel coordinates
    x = fmaxf(0, fminf(faces[faceIndex].width - 1, x));
    y = fmaxf(0, fminf(faces[faceIndex].height - 1, y));
    
    return GetImageColor(faces[faceIndex], x, y);
}

// Sample cubemap with bilinear filtering
Color IBLManager::SampleCubemapBilinear(const Image faces[6], Vector3 direction) {
    int faceIndex;
    float u, v;
    DirectionToCubemapUV(direction, &faceIndex, &u, &v);
    
    // Clamp UV coordinates
    u = fmaxf(0.0f, fminf(1.0f, u));
    v = fmaxf(0.0f, fminf(1.0f, v));
    
    // Get bilinear interpolation coordinates
    float fx = u * (faces[faceIndex].width - 1);
    float fy = v * (faces[faceIndex].height - 1);
    
    int x0 = (int)fx;
    int y0 = (int)fy;
    int x1 = fminf(x0 + 1, faces[faceIndex].width - 1);
    int y1 = fminf(y0 + 1, faces[faceIndex].height - 1);
    
    float tx = fx - x0;
    float ty = fy - y0;
    
    // Sample 4 pixels
    Color c00 = GetImageColor(faces[faceIndex], x0, y0);
    Color c10 = GetImageColor(faces[faceIndex], x1, y0);
    Color c01 = GetImageColor(faces[faceIndex], x0, y1);
    Color c11 = GetImageColor(faces[faceIndex], x1, y1);
    
    // Bilinear interpolation
    float r = (1-tx)*(1-ty)*c00.r + tx*(1-ty)*c10.r + (1-tx)*ty*c01.r + tx*ty*c11.r;
    float g = (1-tx)*(1-ty)*c00.g + tx*(1-ty)*c10.g + (1-tx)*ty*c01.g + tx*ty*c11.g;
    float b = (1-tx)*(1-ty)*c00.b + tx*(1-ty)*c10.b + (1-tx)*ty*c01.b + tx*ty*c11.b;
    
    return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
}

// Convert Color to Vector3 (with sRGB to linear conversion for JPG)
Vector3 IBLManager::ColorToVector3(Color c) {
    Vector3 v;
    v.x = powf(c.r / 255.0f, 2.2f); // sRGB to linear
    v.y = powf(c.g / 255.0f, 2.2f);
    v.z = powf(c.b / 255.0f, 2.2f);
    return v;
}

// Convert Vector3 to Color (with linear to sRGB conversion)
Color IBLManager::Vector3ToColor(Vector3 v) {
    // Clamp values
    v.x = fmaxf(0.0f, fminf(1.0f, v.x));
    v.y = fmaxf(0.0f, fminf(1.0f, v.y));
    v.z = fmaxf(0.0f, fminf(1.0f, v.z));
    
    // Linear to sRGB
    return (Color){
        (unsigned char)(powf(v.x, 1.0f/2.2f) * 255.0f),
        (unsigned char)(powf(v.y, 1.0f/2.2f) * 255.0f),
        (unsigned char)(powf(v.z, 1.0f/2.2f) * 255.0f),
        255
    };
}

// Radical inverse for Hammersley sequence
float IBLManager::RadicalInverse_VdC(unsigned int bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

// Hammersley low-discrepancy sequence
Vector2 IBLManager::Hammersley(unsigned int i, unsigned int N) {
    return (Vector2){float(i) / float(N), RadicalInverse_VdC(i)};
}

// Generate hemisphere sample for irradiance convolution
Vector3 IBLManager::GetHemisphereSample(int i, int numSamples, Vector3 normal) {
    // Uniform hemisphere sampling with cosine weighting
    float u1 = float(i) / float(numSamples);
    float u2 = float((i * 7 + 13) % numSamples) / float(numSamples);
    
    float phi = 2.0f * PI * u1;
    float cosTheta = sqrtf(u2);
    float sinTheta = sqrtf(1.0f - u2);
    
    // Spherical to Cartesian (in tangent space)
    Vector3 tangentSample;
    tangentSample.x = sinTheta * cosf(phi);
    tangentSample.y = sinTheta * sinf(phi);
    tangentSample.z = cosTheta;
    
    // Create tangent space basis from normal
    Vector3 up = fabsf(normal.z) < 0.999f ? (Vector3){0.0f, 0.0f, 1.0f} : (Vector3){1.0f, 0.0f, 0.0f};
    Vector3 tangent = Vector3Normalize(Vector3CrossProduct(up, normal));
    Vector3 bitangent = Vector3CrossProduct(normal, tangent);
    
    // Transform to world space
    Vector3 worldSample;
    worldSample.x = tangentSample.x * tangent.x + tangentSample.y * bitangent.x + tangentSample.z * normal.x;
    worldSample.y = tangentSample.x * tangent.y + tangentSample.y * bitangent.y + tangentSample.z * normal.y;
    worldSample.z = tangentSample.x * tangent.z + tangentSample.y * bitangent.z + tangentSample.z * normal.z;
    
    return Vector3Normalize(worldSample);
}

// Importance sample GGX distribution
Vector3 IBLManager::ImportanceSampleGGX(Vector2 Xi, Vector3 N, float roughness) {
    float a = roughness * roughness;
    
    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrtf((1.0f - Xi.y) / (1.0f + (a*a - 1.0f) * Xi.y));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    
    // Spherical to Cartesian (in tangent space)
    Vector3 H;
    H.x = sinTheta * cosf(phi);
    H.y = sinTheta * sinf(phi);
    H.z = cosTheta;
    
    // Tangent space to world space
    Vector3 up = fabsf(N.z) < 0.999f ? (Vector3){0.0f, 0.0f, 1.0f} : (Vector3){1.0f, 0.0f, 0.0f};
    Vector3 tangent = Vector3Normalize(Vector3CrossProduct(up, N));
    Vector3 bitangent = Vector3CrossProduct(N, tangent);
    
    Vector3 sampleVec;
    sampleVec.x = tangent.x * H.x + bitangent.x * H.y + N.x * H.z;
    sampleVec.y = tangent.y * H.x + bitangent.y * H.y + N.y * H.z;
    sampleVec.z = tangent.z * H.x + bitangent.z * H.y + N.z * H.z;
    
    return Vector3Normalize(sampleVec);
}

// Geometry function for BRDF
float IBLManager::GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0f;
    
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return nom / denom;
}

float IBLManager::GeometrySmith(float NdotV, float NdotL, float roughness) {
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// ============================================================================
// DISK CACHING FOR IBL MAPS
// ============================================================================

bool IBLManager::LoadCachedIBLMaps(const std::string& envName) {
    std::string cacheDir = "hdr/" + envName + "/cache/";
    
    std::cout << "Checking for cached IBL maps... " << std::flush;
    
    // Build path strings
    std::string irradiancePathStrs[6] = {
        cacheDir + "irradiance_posx.png",
        cacheDir + "irradiance_negx.png",
        cacheDir + "irradiance_posy.png",
        cacheDir + "irradiance_negy.png",
        cacheDir + "irradiance_posz.png",
        cacheDir + "irradiance_negz.png"
    };
    
    std::string prefilterPathStrs[6] = {
        cacheDir + "prefilter_posx.png",
        cacheDir + "prefilter_negx.png",
        cacheDir + "prefilter_posy.png",
        cacheDir + "prefilter_negy.png",
        cacheDir + "prefilter_posz.png",
        cacheDir + "prefilter_negz.png"
    };
    
    std::string brdfPath = cacheDir + "brdf_lut.png";
    
    // Try to load irradiance map
    Image irradianceFaces[6];
    bool irradianceLoaded = true;
    for (int i = 0; i < 6; i++) {
        irradianceFaces[i] = LoadImage(irradiancePathStrs[i].c_str());
        if (irradianceFaces[i].data == nullptr) {
            irradianceLoaded = false;
            break;
        }
    }
    
    // Try to load prefilter map
    Image prefilterFaces[6];
    bool prefilterLoaded = true;
    if (irradianceLoaded) {
        for (int i = 0; i < 6; i++) {
            prefilterFaces[i] = LoadImage(prefilterPathStrs[i].c_str());
            if (prefilterFaces[i].data == nullptr) {
                prefilterLoaded = false;
                break;
            }
        }
    }else {
        prefilterLoaded = false;
    }
    
    // Try to load BRDF LUT
    Image brdfImage = {0};
    bool brdfLoaded = false;
    if (irradianceLoaded && prefilterLoaded) {
        brdfImage = LoadImage(brdfPath.c_str());
        brdfLoaded = (brdfImage.data != nullptr);
    }
    
    if (!irradianceLoaded || !prefilterLoaded || !brdfLoaded) {
        std::cout << "Not found or incomplete" << std::endl;
        std::cerr << "DEBUG: About to cleanup and return false from LoadCachedIBLMaps" << std::endl;
        
        // Cleanup any loaded images
        if (irradianceLoaded) {
            for (int i = 0; i < 6; i++) UnloadImage(irradianceFaces[i]);
        }
        if (prefilterLoaded) {
            for (int i = 0; i < 6; i++) UnloadImage(prefilterFaces[i]);
        }
        if (brdfLoaded) UnloadImage(brdfImage);
        
        std::cerr << "DEBUG: Returning false from LoadCachedIBLMaps" << std::endl;
        std::cerr.flush();
        return false;
    }
    
    std::cout << "Found! Loading from cache..." << std::endl;
    std::cout.flush();
    
    // Upload irradiance map
    int irradianceSize = irradianceFaces[0].width;
    int faceSize = irradianceSize * irradianceSize;
    int bytesPerPixel = 4;
    int totalSize = faceSize * bytesPerPixel * 6;
    
    unsigned char* combinedData = (unsigned char*)malloc(totalSize);
    if (combinedData) {
        for (int i = 0; i < 6; i++) {
            memcpy(combinedData + (i * faceSize * bytesPerPixel), 
                   irradianceFaces[i].data, 
                   faceSize * bytesPerPixel);
        }
        
        irradianceMap.id = rlLoadTextureCubemap(combinedData, irradianceSize, irradianceFaces[0].format, 1);
        irradianceMap.width = irradianceSize;
        irradianceMap.height = irradianceSize;
        irradianceMap.mipmaps = 1;
        irradianceMap.format = irradianceFaces[0].format;
        
        free(combinedData);
    }
    
    for (int i = 0; i < 6; i++) UnloadImage(irradianceFaces[i]);
    
    // Upload prefilter map
    int prefilterSize = prefilterFaces[0].width;
    faceSize = prefilterSize * prefilterSize;
    totalSize = faceSize * bytesPerPixel * 6;
    
    combinedData = (unsigned char*)malloc(totalSize);
    if (combinedData) {
        for (int i = 0; i < 6; i++) {
            memcpy(combinedData + (i * faceSize * bytesPerPixel), 
                   prefilterFaces[i].data, 
                   faceSize * bytesPerPixel);
        }
        
        prefilterMap.id = rlLoadTextureCubemap(combinedData, prefilterSize, prefilterFaces[0].format, 1);
        prefilterMap.width = prefilterSize;
        prefilterMap.height = prefilterSize;
        prefilterMap.mipmaps = 1;
        prefilterMap.format = prefilterFaces[0].format;
        
        // Generate mipmaps
        rlEnableTextureCubemap(prefilterMap.id);
        GenTextureMipmaps(&prefilterMap);
        
        free(combinedData);
    }
    
    for (int i = 0; i < 6; i++) UnloadImage(prefilterFaces[i]);
    
    // Upload BRDF LUT
    brdfLUT = LoadTextureFromImage(brdfImage);
    UnloadImage(brdfImage);
    
    mapsGenerated = true;
    std::cout << "IBL maps loaded from cache successfully!" << std::endl;
    
    return true;
}

void IBLManager::SaveIBLMapsToCache(const std::string& envName) {
    if (!mapsGenerated) return;
    
    std::cout << "Saving IBL maps to cache... " << std::flush;
    
    std::string cacheDir = "hdr/" + envName + "/cache/";
    
    // Create cache directory (platform-specific, this is a simple approach)
    #ifdef _WIN32
        std::string mkdirCmd = "mkdir \"" + cacheDir + "\" 2>nul";
    #else
        std::string mkdirCmd = "mkdir -p \"" + cacheDir + "\"";
    #endif
    system(mkdirCmd.c_str());
    
    // Save irradiance map faces
    // Note: We need to download from GPU to save, which is complex
    // For now, we'll skip saving since we're generating at runtime
    // In a production app, you'd implement proper GPU->CPU readback
    
    std::cout << "Skipped (not fully implemented)" << std::endl;
    
    // TODO: Implement GPU texture readback and save to PNG files
    // This requires using glGetTexImage or similar, which is not exposed by raylib
}

// ============================================================================
// ENVIRONMENT MAP LOADING
// ============================================================================

void IBLManager::GenerateDefaultEnvironment() {
    // Try to load from cache first
    if (LoadCachedIBLMaps("Storforsen3")) {
        // Cache loaded successfully, now load environment map for skybox
        const char* facePaths[6] = {
            "hdr/Storforsen3/posx.jpg",
            "hdr/Storforsen3/negx.jpg",
            "hdr/Storforsen3/posy.jpg",
            "hdr/Storforsen3/negy.jpg",
            "hdr/Storforsen3/posz.jpg",
            "hdr/Storforsen3/negz.jpg"
        };
        
        Image faces[6] = {0};
        bool allLoaded = true;
        int loadedCount = 0;
        
        for (int i = 0; i < 6; i++) {
            faces[i] = LoadImage(facePaths[i]);
            if (faces[i].data == nullptr) {
                allLoaded = false;
                break;
            }
            loadedCount++;
        }
        
        if (allLoaded) {
            int faceSize = faces[0].width * faces[0].height;
            int bytesPerPixel = 3;
            int totalSize = faceSize * bytesPerPixel * 6;
            
            unsigned char* combinedData = (unsigned char*)malloc(totalSize);
            if (combinedData) {
                for (int i = 0; i < 6; i++) {
                    memcpy(combinedData + (i * faceSize * bytesPerPixel), 
                           faces[i].data, 
                           faceSize * bytesPerPixel);
                }
                
                environmentMap.id = rlLoadTextureCubemap(combinedData, faces[0].width, faces[0].format, 1);
                environmentMap.width = faces[0].width;
                environmentMap.height = faces[0].height;
                environmentMap.mipmaps = 1;
                environmentMap.format = faces[0].format;
                
                free(combinedData);
            }
            
            for (int i = 0; i < 6; i++) {
                UnloadImage(faces[i]);
            }
            
            environmentLoaded = true;
            return;
        }
        
        // Cleanup if loading failed
        for (int i = 0; i < loadedCount; i++) {
            UnloadImage(faces[i]);
        }
    }
    
    std::cerr << "CHECKPOINT 1: After cache load block" << std::endl;
    std::cout << "Cache not loaded, proceeding to load and generate IBL maps..." << std::endl;
    
    // Load the 6 cubemap faces from disk
    std::cerr << "CHECKPOINT 2: Starting to load environment faces" << std::endl;
    std::cout << "Loading environment cubemap faces..." << std::endl;
    const char* facePaths[6] = {
        "hdr/Storforsen3/posx.jpg",  // Right  (+X)
        "hdr/Storforsen3/negx.jpg",  // Left   (-X)
        "hdr/Storforsen3/posy.jpg",  // Top    (+Y)
        "hdr/Storforsen3/negy.jpg",  // Bottom (-Y)
        "hdr/Storforsen3/posz.jpg",  // Front  (+Z)
        "hdr/Storforsen3/negz.jpg"   // Back   (-Z)
    };
    
    // Load all 6 face images
    Image faces[6] = {0};
    bool allLoaded = true;
    int loadedCount = 0;
    
    for (int i = 0; i < 6; i++) {
        faces[i] = LoadImage(facePaths[i]);
        if (faces[i].data == nullptr) {
            std::cerr << "Failed to load cubemap face: " << facePaths[i] << std::endl;
            allLoaded = false;
            break;
        }
        loadedCount++;
    }
    
    if (!allLoaded) {
        // Clean up only loaded faces
        for (int i = 0; i < loadedCount; i++) {
            UnloadImage(faces[i]);
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
    
    std::cout << "\n=== STARTING IBL GENERATION ===" << std::endl;
    std::cout << "This will take approximately 2-3 seconds..." << std::endl;
    
    // Generate IBL maps from loaded environment
    GenerateIrradianceMap();
    GeneratePrefilterMap();
    GenerateBRDFLUT();
    
    std::cout << "\n=== IBL GENERATION COMPLETE ===" << std::endl;
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
    
    std::cout << "Generating irradiance map from environment... " << std::flush;
    
    // Load environment faces for sampling
    const char* facePaths[6] = {
        "hdr/Storforsen3/posx.jpg",
        "hdr/Storforsen3/negx.jpg",
        "hdr/Storforsen3/posy.jpg",
        "hdr/Storforsen3/negy.jpg",
        "hdr/Storforsen3/posz.jpg",
        "hdr/Storforsen3/negz.jpg"
    };
    
    Image envFaces[6] = {0};
    bool allLoaded = true;
    int loadedCount = 0;
    for (int i = 0; i < 6; i++) {
        envFaces[i] = LoadImage(facePaths[i]);
        if (envFaces[i].data == nullptr) {
            std::cerr << "Failed to load face: " << facePaths[i] << std::endl;
            allLoaded = false;
            break;
        }
        loadedCount++;
    }
    
    if (!allLoaded) {
        for (int i = 0; i < loadedCount; i++) {
            UnloadImage(envFaces[i]);
        }
        std::cerr << "Failed to generate irradiance map" << std::endl;
        return;
    }
    
    // Create irradiance map (32x32 per face)
    const int size = 32;
    const int numSamples = 256; // Low quality
    Image irradianceFaces[6];
    
    for (int face = 0; face < 6; face++) {
        irradianceFaces[face] = GenImageColor(size, size, BLACK);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                // Get direction for this texel
                float u = (x + 0.5f) / size;
                float v = (y + 0.5f) / size;
                Vector3 normal = GetCubemapDirection(face, u, v);
                
                // Convolve hemisphere
                Vector3 irradiance = {0.0f, 0.0f, 0.0f};
                
                for (int i = 0; i < numSamples; i++) {
                    Vector3 sampleDir = GetHemisphereSample(i, numSamples, normal);
                    Color sampleColor = SampleCubemapBilinear(envFaces, sampleDir);
                    Vector3 sampleLinear = ColorToVector3(sampleColor);
                    
                    float NdotL = fmaxf(Vector3DotProduct(normal, sampleDir), 0.0f);
                    irradiance.x += sampleLinear.x * NdotL;
                    irradiance.y += sampleLinear.y * NdotL;
                    irradiance.z += sampleLinear.z * NdotL;
                }
                
                // Average
                irradiance.x /= numSamples;
                irradiance.y /= numSamples;
                irradiance.z /= numSamples;
                
                // Store
                ImageDrawPixel(&irradianceFaces[face], x, y, Vector3ToColor(irradiance));
            }
        }
    }
    
    // Combine and upload
    int faceSize = size * size;
    int bytesPerPixel = 4;
    int totalSize = faceSize * bytesPerPixel * 6;
    
    unsigned char* combinedData = (unsigned char*)malloc(totalSize);
    if (combinedData) {
        for (int i = 0; i < 6; i++) {
            memcpy(combinedData + (i * faceSize * bytesPerPixel), 
                   irradianceFaces[i].data, 
                   faceSize * bytesPerPixel);
        }
        
        irradianceMap.id = rlLoadTextureCubemap(combinedData, size, irradianceFaces[0].format, 1);
        irradianceMap.width = size;
        irradianceMap.height = size;
        irradianceMap.mipmaps = 1;
        irradianceMap.format = irradianceFaces[0].format;
        
        free(combinedData);
    }
    
    // Cleanup
    for (int i = 0; i < 6; i++) {
        UnloadImage(envFaces[i]);
        UnloadImage(irradianceFaces[i]);
    }
    
    std::cout << "Done!" << std::endl;
}

void IBLManager::GeneratePrefilterMap() {
    if (!environmentLoaded) return;
    
    std::cout << "Generating prefilter map from environment... " << std::flush;
    
    // Load environment faces for sampling
    const char* facePaths[6] = {
        "hdr/Storforsen3/posx.jpg",
        "hdr/Storforsen3/negx.jpg",
        "hdr/Storforsen3/posy.jpg",
        "hdr/Storforsen3/negy.jpg",
        "hdr/Storforsen3/posz.jpg",
        "hdr/Storforsen3/negz.jpg"
    };
    
    Image envFaces[6] = {0};
    bool allLoaded = true;
    int loadedCount = 0;
    for (int i = 0; i < 6; i++) {
        envFaces[i] = LoadImage(facePaths[i]);
        if (envFaces[i].data == nullptr) {
            std::cerr << "Failed to load face: " << facePaths[i] << std::endl;
            allLoaded = false;
            break;
        }
        loadedCount++;
    }
    
    if (!allLoaded) {
        for (int i = 0; i < loadedCount; i++) {
            UnloadImage(envFaces[i]);
        }
        std::cerr << "Failed to generate prefilter map" << std::endl;
        return;
    }
    
    // Create prefilter map (128x128 base, we'll generate mip 0 only and let GPU handle mipmaps)
    const int size = 128;
    const int numSamples = 256; // Low quality
    const float roughness = 0.0f; // Base mip level
    
    Image prefilterFaces[6];
    
    for (int face = 0; face < 6; face++) {
        prefilterFaces[face] = GenImageColor(size, size, BLACK);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                // Get direction for this texel
                float u = (x + 0.5f) / size;
                float v = (y + 0.5f) / size;
                Vector3 N = GetCubemapDirection(face, u, v);
                Vector3 R = N;
                Vector3 V = R;
                
                Vector3 prefilteredColor = {0.0f, 0.0f, 0.0f};
                float totalWeight = 0.0f;
                
                for (unsigned int i = 0; i < numSamples; i++) {
                    Vector2 Xi = Hammersley(i, numSamples);
                    Vector3 H = ImportanceSampleGGX(Xi, N, roughness);
                    
                    // Calculate L from H
                    float VdotH = Vector3DotProduct(V, H);
                    Vector3 L = {
                        2.0f * VdotH * H.x - V.x,
                        2.0f * VdotH * H.y - V.y,
                        2.0f * VdotH * H.z - V.z
                    };
                    
                    float NdotL = fmaxf(Vector3DotProduct(N, L), 0.0f);
                    if (NdotL > 0.0f) {
                        Color sampleColor = SampleCubemapBilinear(envFaces, L);
                        Vector3 sampleLinear = ColorToVector3(sampleColor);
                        
                        prefilteredColor.x += sampleLinear.x * NdotL;
                        prefilteredColor.y += sampleLinear.y * NdotL;
                        prefilteredColor.z += sampleLinear.z * NdotL;
                        totalWeight += NdotL;
                    }
                }
                
                if (totalWeight > 0.0f) {
                    prefilteredColor.x /= totalWeight;
                    prefilteredColor.y /= totalWeight;
                    prefilteredColor.z /= totalWeight;
                }
                
                ImageDrawPixel(&prefilterFaces[face], x, y, Vector3ToColor(prefilteredColor));
            }
        }
    }
    
    // Combine and upload
    int faceSize = size * size;
    int bytesPerPixel = 4;
    int totalSize = faceSize * bytesPerPixel * 6;
    
    unsigned char* combinedData = (unsigned char*)malloc(totalSize);
    if (combinedData) {
        for (int i = 0; i < 6; i++) {
            memcpy(combinedData + (i * faceSize * bytesPerPixel), 
                   prefilterFaces[i].data, 
                   faceSize * bytesPerPixel);
        }
        
        prefilterMap.id = rlLoadTextureCubemap(combinedData, size, prefilterFaces[0].format, 1);
        prefilterMap.width = size;
        prefilterMap.height = size;
        prefilterMap.mipmaps = 1;
        prefilterMap.format = prefilterFaces[0].format;
        
        // Generate mipmaps for different roughness levels
        rlEnableTextureCubemap(prefilterMap.id);
        GenTextureMipmaps(&prefilterMap);
        
        free(combinedData);
    }
    
    // Cleanup
    for (int i = 0; i < 6; i++) {
        UnloadImage(envFaces[i]);
        UnloadImage(prefilterFaces[i]);
    }
    
    std::cout << "Done!" << std::endl;
}

void IBLManager::GenerateBRDFLUT() {
    std::cout << "Generating BRDF LUT... " << std::flush;
    
    // Generate BRDF integration lookup texture
    const int size = 512;
    const int numSamples = 256; // Low quality
    
    Image brdfImage = GenImageColor(size, size, BLACK);
    
    for (int y = 0; y < size; y++) {
        float roughness = (float)y / (float)(size - 1);
        
        for (int x = 0; x < size; x++) {
            float NdotV = (float)x / (float)(size - 1);
            NdotV = fmaxf(NdotV, 0.001f); // Avoid zero
            
            // View vector (pointing toward camera)
            Vector3 V = {sqrtf(1.0f - NdotV * NdotV), 0.0f, NdotV};
            Vector3 N = {0.0f, 0.0f, 1.0f};
            
            float A = 0.0f;
            float B = 0.0f;
            
            for (unsigned int i = 0; i < numSamples; i++) {
                Vector2 Xi = Hammersley(i, numSamples);
                Vector3 H = ImportanceSampleGGX(Xi, N, roughness);
                
                // Calculate L
                float VdotH = Vector3DotProduct(V, H);
                Vector3 L = {
                    2.0f * VdotH * H.x - V.x,
                    2.0f * VdotH * H.y - V.y,
                    2.0f * VdotH * H.z - V.z
                };
                
                float NdotL = fmaxf(L.z, 0.0f);
                float NdotH = fmaxf(H.z, 0.0f);
                VdotH = fmaxf(VdotH, 0.0f);
                
                if (NdotL > 0.0f) {
                    // Geometry term
                    float G = GeometrySmith(NdotV, NdotL, roughness);
                    float G_Vis = (G * VdotH) / (NdotH * NdotV);
                    float Fc = powf(1.0f - VdotH, 5.0f);
                    
                    A += (1.0f - Fc) * G_Vis;
                    B += Fc * G_Vis;
                }
            }
            
            A /= numSamples;
            B /= numSamples;
            
            // Clamp values
            A = fmaxf(0.0f, fminf(1.0f, A));
            B = fmaxf(0.0f, fminf(1.0f, B));
            
            Color lutColor = {
                (unsigned char)(A * 255.0f),
                (unsigned char)(B * 255.0f),
                0,
                255
            };
            ImageDrawPixel(&brdfImage, x, y, lutColor);
        }
    }
    
    brdfLUT = LoadTextureFromImage(brdfImage);
    UnloadImage(brdfImage);
    
    std::cout << "Done!" << std::endl;
    
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
    Matrix projMatrix = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1, 1000.0);
    
    // Calculate MVP: projection * view (no scale in MVP)
    Matrix mvpMatrix = MatrixMultiply(viewMatrix, projMatrix);
    
    SetShaderValueMatrix(skyboxShader, skyboxShader.locs[SHADER_LOC_MATRIX_MVP], mvpMatrix);
    
    // Create material with cubemap texture assigned
    Material skyboxMat = LoadMaterialDefault();
    skyboxMat.shader = skyboxShader;
    skyboxMat.maps[MATERIAL_MAP_CUBEMAP].texture = environmentMap;
    
    // Draw skybox with large scale transform (applied before MVP in model space)
    Matrix skyboxTransform = MatrixScale(500.0f, 500.0f, 500.0f);
    DrawMesh(skyboxMesh, skyboxMat, skyboxTransform);
    
    // Don't unload material - it would unload our shader
    // The material is lightweight and only allocated on stack
    
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

} // namespace AAV

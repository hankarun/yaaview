#pragma once

#include "raylib.h"
#include <string>

namespace AAV {

class IBLManager {
public:
    IBLManager();
    ~IBLManager();
    
    bool Initialize();
    void Cleanup();
    
    // Environment map loading
    bool LoadEnvironmentMap(const std::string& hdriPath);
    void GenerateDefaultEnvironment();
    
    // IBL map generation
    void GenerateIrradianceMap();
    void GeneratePrefilterMap();
    void GenerateBRDFLUT();
    
    // Skybox rendering
    void RenderSkybox(Camera3D camera, int viewportWidth, int viewportHeight, float exposure = 1.0f);
    
    // Getters
    TextureCubemap GetIrradianceMap() const { return irradianceMap; }
    TextureCubemap GetPrefilterMap() const { return prefilterMap; }
    Texture2D GetBRDFLUT() const { return brdfLUT; }
    TextureCubemap GetEnvironmentMap() const { return environmentMap; }
    
    bool IsLoaded() const { return environmentLoaded; }
    bool IsSkyboxEnabled() const { return skyboxEnabled; }
    void SetSkyboxEnabled(bool enabled) { skyboxEnabled = enabled; }
    
private:
    // Cubemap generation from equirectangular
    TextureCubemap LoadCubemapFromEquirectangular(const std::string& hdriPath);
    void GenerateFallbackEnvironment();
    
    // Disk caching
    bool LoadCachedIBLMaps(const std::string& envName);
    void SaveIBLMapsToCache(const std::string& envName);
    
    // Cubemap sampling utilities
    Color SampleEnvironmentMap(const Image faces[6], Vector3 direction);
    Vector3 GetCubemapDirection(int face, float u, float v);
    void DirectionToCubemapUV(Vector3 direction, int* faceIndex, float* u, float* v);
    Color SampleCubemapBilinear(const Image faces[6], Vector3 direction);
    
    // Hemisphere sampling for convolution
    Vector3 GetHemisphereSample(int i, int numSamples, Vector3 normal);
    Vector3 ImportanceSampleGGX(Vector2 Xi, Vector3 N, float roughness);
    
    // Math utilities
    float RadicalInverse_VdC(unsigned int bits);
    Vector2 Hammersley(unsigned int i, unsigned int N);
    Vector3 ColorToVector3(Color c);
    Color Vector3ToColor(Vector3 v);
    float GeometrySchlickGGX(float NdotV, float roughness);
    float GeometrySmith(float NdotV, float NdotL, float roughness);
    
    // Environment maps
    TextureCubemap environmentMap;     // Original HDR cubemap
    TextureCubemap irradianceMap;      // Diffuse convolution
    TextureCubemap prefilterMap;       // Specular prefiltered
    Texture2D brdfLUT;                 // BRDF integration map
    
    // Skybox rendering
    Shader skyboxShader;
    Mesh skyboxMesh;
    bool skyboxEnabled;
    
    // State
    bool environmentLoaded;
    bool mapsGenerated;
};

} // namespace AAV

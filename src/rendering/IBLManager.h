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

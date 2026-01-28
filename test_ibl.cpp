#include "src/rendering/IBLManager.h"
#include "raylib.h"
#include <iostream>

int main() {
    std::cout << "Testing IBL Generation..." << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Initialize raylib (needed for image/texture functions)
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(800, 600, "IBL Test");
    
    // Create IBL Manager
    AAV::IBLManager iblManager;
    
    // Initialize (this will trigger IBL generation)
    std::cout << "\nInitializing IBL Manager..." << std::endl;
    bool success = iblManager.Initialize();
    
    if (success) {
        std::cout << "\n=====================================" << std::endl;
        std::cout << "IBL Generation SUCCESSFUL!" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Environment loaded: " << (iblManager.IsLoaded() ? "YES" : "NO") << std::endl;
        std::cout << "Irradiance map ID: " << iblManager.GetIrradianceMap().id << std::endl;
        std::cout << "Prefilter map ID: " << iblManager.GetPrefilterMap().id << std::endl;
        std::cout << "BRDF LUT ID: " << iblManager.GetBRDFLUT().id << std::endl;
    } else {
        std::cout << "\n=====================================" << std::endl;
        std::cout << "IBL Generation FAILED!" << std::endl;
        std::cout << "=====================================" << std::endl;
    }
    
    // Cleanup
    CloseWindow();
    
    return success ? 0 : 1;
}

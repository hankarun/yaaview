#include "Application.h"
#include <iostream>

int main(int argc, char* argv[])
{
    AAV::Application app;
    
    if (!app.Initialize(1280, 720, "AAV - Assimp Advanced Viewer")) {
        std::cerr << "Failed to initialize application!" << std::endl;
        return 1;
    }
    
    std::cout << "==================================" << std::endl;
    std::cout << "   AAV - Assimp Advanced Viewer   " << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Use File > Open Model to load a 3D model" << std::endl;
    std::cout << "\nSupported formats:" << std::endl;
    std::cout << "  - OBJ, FBX, GLTF/GLB" << std::endl;
    std::cout << "  - DAE, 3DS, BLEND, STL" << std::endl;
    std::cout << "\nCamera controls:" << std::endl;
    std::cout << "  - Left Mouse: Rotate camera" << std::endl;
    std::cout << "  - Right Mouse: Pan camera" << std::endl;
    std::cout << "  - Mouse Wheel: Zoom" << std::endl;
    std::cout << "  - R key: Reset camera" << std::endl;
    std::cout << "==================================" << std::endl;
    
    app.Run();
    
    return 0;
}

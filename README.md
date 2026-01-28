# AAV - Assimp Advanced Viewer

A professional 3D model viewer application built with Raylib, Dear ImGui, and Assimp.

![AAV Screenshot](docs/screenshot.png)

## Features

### Main Components
- **Main Menu Bar**: File operations, view controls, window layouts, and help
- **Scene Window**: Interactive 3D viewport with orbit camera controls
- **Inspector Window**: Detailed model information and transform controls

### Model Support
Supports all Assimp-compatible formats including:
- **OBJ**, **FBX**, **GLTF/GLB**
- **DAE** (Collada), **3DS**
- **BLEND** (Blender), **STL**
- And many more...

### Camera Controls (Scene Window)
- **Left Mouse + Drag**: Rotate camera around model
- **Right Mouse + Drag**: Pan camera
- **Mouse Wheel**: Zoom in/out
- **R Key**: Reset camera to default position

### Inspector Features
- **Model Info**: File name, mesh/material/animation counts, vertex/face statistics, bounding box dimensions
- **Transform**: Position, rotation, and scale controls with uniform scale option
- **Materials**: Material list with diffuse/specular colors and properties
- **Hierarchy**: Scene node structure (basic implementation)
- **Animations**: Animation list with duration info (playback TBD)

## Building

### Prerequisites
- CMake 3.16 or higher
- C++17 compatible compiler
- Assimp library

**macOS:**
```bash
brew install assimp
```

**Linux:**
```bash
sudo apt-get install libassimp-dev
```

**Windows:**
- Download Assimp from https://github.com/assimp/assimp/releases

### Build Steps

```bash
mkdir build
cd build
cmake ..
make -j8

# Run
./bin/AAV
```

## Usage

1. Launch the application
2. Go to **File > Open Model** to load a 3D model
3. Enter the full path to your model file
4. Use the mouse to navigate the 3D view
5. Inspect model properties in the Inspector panel
6. Toggle windows via **View** menu
7. Try different layouts from **Window** menu

## Architecture

```
src/
├── Application.h/.cpp        # Main application controller
├── main.cpp                  # Entry point
├── model/
│   ├── Model.h/.cpp          # Model data structure
│   ├── ModelLoader.h/.cpp    # Assimp model loading
│   └── ModelRenderer.h/.cpp  # Raylib 3D rendering
└── ui/
    ├── MainMenuBar.h/.cpp    # Top menu bar
    ├── SceneWindow.h/.cpp    # 3D viewport with orbit camera
    └── InspectorWindow.h/.cpp # Inspector panel
```

## Technology Stack

- **Raylib 5.5**: Window management and 3D rendering
- **Dear ImGui (Docking)**: UI framework with docking support
- **rlImGui**: Raylib-ImGui integration library
- **Assimp**: 3D model loading and processing

## Future Enhancements

- [ ] Native file dialogs for all platforms
- [ ] Animation playback system with controls
- [ ] Full scene hierarchy tree view
- [ ] Material editor with texture preview
- [ ] Wireframe/shading mode toggles
- [ ] Screenshot capture functionality
- [ ] Layout preset saving/loading
- [ ] Recent files list
- [ ] Texture loading and display
- [ ] Lighting controls
- [ ] Export functionality

## Dependencies

All dependencies except Assimp are fetched automatically via CMake FetchContent:
- **raylib**: https://github.com/raysan5/raylib (v5.5)
- **Dear ImGui**: https://github.com/ocornut/imgui (docking branch)
- **rlImGui**: https://github.com/raylib-extras/rlImGui
- **Assimp**: System-installed via package manager

## License

MIT License

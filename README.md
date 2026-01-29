# AAV - Yet Another Assimp Viewer

<div align="center">

**A professional 3D model viewer application built with Raylib, Dear ImGui, and Assimp**

[![CMake](https://img.shields.io/badge/CMake-3.16+-blue.svg)](https://cmake.org/)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)](https://github.com)

</div>

---

## 📸 Screenshot

![AAV Screenshot](docs/screenshot.png)
*3D model viewer with integrated Inspector, Hierarchy, and Scene windows*

---

## ✨ Features

### 🎨 Main Components
- **Main Menu Bar**: File operations, view controls, window layouts, and help
- **Scene Window**: Interactive 3D viewport with orbit camera controls
- **Inspector Window**: Detailed model information and transform controls
- **Hierarchy Window**: Navigate through scene node structure
- **Light Window**: Adjust lighting parameters
- **Settings Window**: Configure application preferences
- **Log Window**: View application messages and debug information
- **Model Info Window**: Display comprehensive model statistics

### 📦 Model Support
Supports all **Assimp-compatible formats** including:
- **Common**: OBJ, FBX, GLTF/GLB, DAE (Collada)
- **CAD**: 3DS, STL, PLY
- **Game Engines**: BLEND (Blender), X (DirectX)
- **And 40+ more formats...**

### 🎮 Camera Controls (Scene Window)
| Action | Control |
|--------|---------|
| Rotate camera | Left Mouse + Drag |
| Pan camera | Right Mouse + Drag |
| Zoom in/out | Mouse Wheel |
| Reset camera | R Key |

### 🔍 Inspector Features
- **Model Info Tab**
  - File name and path
  - Mesh, material, and animation counts
  - Vertex and face statistics
  - Bounding box dimensions
  
- **Transform Tab**
  - Position, rotation, and scale controls
  - Uniform scale option
  - Real-time manipulation

- **Materials Tab**
  - Material list with properties
  - Diffuse and specular colors
  - Texture information

- **Hierarchy Tab**
  - Scene node structure visualization
  - Node selection and inspection

- **Animations Tab**
  - Animation list with duration info
  - Animation playback (planned)

---

## 🚀 Getting Started

### Prerequisites

#### Required
- **CMake** 3.16 or higher
- **C++17** compatible compiler (MSVC 2019+, GCC 7+, Clang 5+)
- **Git** (for fetching dependencies)

#### Platform-Specific

**Windows:**
- Visual Studio 2019 or later (with C++ Desktop Development workload)
- All dependencies are fetched automatically via CMake

**macOS:**
```bash
# Install Assimp via Homebrew
brew install assimp
```

**Linux (Ubuntu/Debian):**
```bash
# Install required packages
sudo apt-get update
sudo apt-get install build-essential cmake git libassimp-dev
```

**Linux (Fedora):**
```bash
sudo dnf install cmake gcc-c++ git assimp-devel
```

### 🔨 Building from Source

#### Windows (Visual Studio)
```powershell
# Clone the repository
git clone https://github.com/yourusername/yaaview.git
cd yaaview

# Create build directory
mkdir build
cd build

# Generate Visual Studio solution
cmake ..

# Build (or open AAV.sln in Visual Studio)
cmake --build . --config Release

# Run
.\bin\Release\AAV.exe
```

#### macOS / Linux
```bash
# Clone the repository
git clone https://github.com/yourusername/yaaview.git
cd yaaview

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j8

# Run
./bin/AAV
```

### 📦 Dependencies

All dependencies are managed via CMake FetchContent (except Assimp on macOS/Linux):

| Library | Version | Purpose | Auto-Fetched |
|---------|---------|---------|--------------|
| [Raylib](https://github.com/raysan5/raylib) | 5.5 | 3D rendering & windowing | ✅ Yes |
| [Dear ImGui](https://github.com/ocornut/imgui) | Docking | UI framework | ✅ Yes |
| [rlImGui](https://github.com/raylib-extras/rlImGui) | Latest | Raylib-ImGui bridge | ✅ Yes |
| [Assimp](https://github.com/assimp/assimp) | 6.0.4 | 3D model loading | ✅ Windows, ❌ macOS/Linux |
| [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) | Latest | File dialogs | ✅ Yes |

---

## 📖 Usage

### Basic Workflow

1. **Launch the Application**
   ```bash
   # Windows
   .\bin\Release\AAV.exe
   
   # macOS/Linux
   ./bin/AAV
   ```

2. **Load a 3D Model**
   - Click `File > Open Model` in the menu bar
   - Select your model file from the dialog
   - The model will load and center in the Scene window

3. **Navigate the 3D View**
   - Use mouse controls to orbit, pan, and zoom
   - Press `R` to reset the camera to default position

4. **Inspect Model Properties**
   - View mesh statistics in the **Model Info** window
   - Adjust transforms in the **Inspector** window
   - Explore the scene hierarchy in the **Hierarchy** window
   - Configure lighting in the **Light** window

5. **Customize Layout**
   - Toggle windows via `View` menu
   - Use `Window` menu for preset layouts
   - Dock windows by dragging their title bars

### Window Management

| Window | Description | Shortcut |
|--------|-------------|----------|
| Scene | 3D viewport with camera controls | - |
| Inspector | Transform and material properties | - |
| Hierarchy | Scene node tree structure | - |
| Model Info | Statistics and bounding box | - |
| Light | Lighting parameters | - |
| Settings | Application preferences | - |
| Log | Debug messages and info | - |

---

## 🏗️ Architecture

```
src/
├── main.cpp                     # Application entry point
├── Application.h/.cpp           # Main application controller & event loop
│
├── model/                       # Model data and rendering
│   ├── Model.h/.cpp            # Core model data structure
│   ├── ModelLoader.h/.cpp      # Assimp model loading & parsing
│   └── ModelRenderer.h/.cpp    # Raylib-based 3D rendering
│
├── ui/                          # ImGui-based UI components
│   ├── MainMenuBar.h/.cpp      # Top menu bar with commands
│   ├── SceneWindow.h/.cpp      # 3D viewport with orbit camera
│   ├── InspectorWindow.h/.cpp  # Transform & material inspector
│   ├── HierarchyWindow.h/.cpp  # Scene hierarchy tree
│   ├── ModelInfoWindow.h/.cpp  # Model statistics display
│   ├── LightWindow.h/.cpp      # Lighting controls
│   ├── SettingsWindow.h/.cpp   # Application settings
│   └── LogWindow.h/.cpp        # Debug log viewer
│
└── util/                        # Utility classes
    └── Logger.cpp               # Logging system
```

### Design Patterns
- **Component-based UI**: Each window is an independent component
- **Observer pattern**: Callbacks for menu actions and events
- **MVC-inspired**: Model (data), View (UI), Controller (Application)

---

## 🛠️ Technology Stack

| Technology | Purpose | Version |
|------------|---------|---------|
| **[Raylib](https://www.raylib.com/)** | Window management, input, 3D rendering | 5.5 |
| **[Dear ImGui](https://github.com/ocornut/imgui)** | Immediate mode GUI framework | Docking branch |
| **[rlImGui](https://github.com/raylib-extras/rlImGui)** | Raylib-ImGui integration | Latest |
| **[Assimp](http://www.assimp.org/)** | 3D asset import library | 6.0.4 |
| **[NFD Extended](https://github.com/btzy/nativefiledialog-extended)** | Native file dialogs | Latest |

### Why These Technologies?
- **Raylib**: Lightweight, easy-to-use 3D rendering with minimal dependencies
- **Dear ImGui**: Fast, flexible UI framework perfect for tools and editors
- **Assimp**: Industry-standard model loading supporting 40+ formats
- **NFD**: Native OS file dialogs for seamless user experience

---

## 🎯 Roadmap & Future Enhancements

### High Priority
- [ ] 🎬 Animation playback system with timeline controls
- [ ] 🖼️ Texture loading and preview in material inspector
- [ ] 🌐 Wireframe/shaded/textured rendering modes
- [ ] 💾 Recent files list with quick access
- [ ] 📁 Drag-and-drop file loading

### Medium Priority
- [ ] 🎨 Advanced material editor
- [ ] 💡 Multiple light sources with controls
- [ ] 📊 Advanced scene hierarchy with search
- [ ] 🔧 Export functionality (OBJ, GLTF)
- [ ] 📸 Screenshot capture with transparency
- [ ] 💾 Save/load custom layout presets

### Low Priority
- [ ] 🎨 Shader preview and customization
- [ ] 📏 Measurement tools (distance, angle)
- [ ] 🔍 Model comparison view (side-by-side)
- [ ] 📋 Batch processing capabilities
- [ ] 🌓 Dark/Light theme toggle
- [ ] ⌨️ Customizable keyboard shortcuts

### Completed
- [x] ✅ Native file dialogs for all platforms
- [x] ✅ Multi-window dockable interface
- [x] ✅ Orbit camera with mouse controls
- [x] ✅ Basic material and mesh information display

---

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

1. **Fork the repository**
2. **Create a feature branch** (`git checkout -b feature/AmazingFeature`)
3. **Commit your changes** (`git commit -m 'Add some AmazingFeature'`)
4. **Push to the branch** (`git push origin feature/AmazingFeature`)
5. **Open a Pull Request**

### Development Guidelines
- Follow the existing code style and structure
- Add comments for complex logic
- Test on your target platform before submitting
- Update documentation for new features

---

## 🐛 Known Issues

- **macOS**: Assimp must be installed via Homebrew (not auto-fetched)
- **Linux**: Some distributions may require additional OpenGL libraries
- **All Platforms**: Animation playback not yet implemented
- **All Platforms**: Texture display in materials is placeholder-only

---

## 📝 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## 👏 Acknowledgments

- **[Raylib](https://www.raylib.com/)** - Ramon Santamaria (@raysan5)
- **[Dear ImGui](https://github.com/ocornut/imgui)** - Omar Cornut (@ocornut)
- **[Assimp](http://www.assimp.org/)** - Open Asset Import Library team
- **[rlImGui](https://github.com/raylib-extras/rlImGui)** - Jeff Olsen (@raylib-extras)
- **[NFD Extended](https://github.com/btzy/nativefiledialog-extended)** - Bernard Teo (@btzy)

---

## 📧 Contact & Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/yaaview/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/yaaview/discussions)

---

<div align="center">

**Made with ❤️ using C++, Raylib, and Dear ImGui**

⭐ Star this repository if you find it helpful!

</div>

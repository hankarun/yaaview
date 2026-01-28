cmake_minimum_required(VERSION 3.16)
project(AAV VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Ignore homebrew paths to avoid broken minizip
list(APPEND CMAKE_IGNORE_PATH "/opt/homebrew/Cellar/minizip" "/opt/homebrew/lib" "/opt/homebrew/include")

# Include FetchContent module
include(FetchContent)

# Fetch raylib 5.5
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Fetch Dear ImGui (required for rlImGui)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Fetch rlImGui
FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG main
)
FetchContent_MakeAvailable(rlimgui)

# Fetch nativefiledialog-extended
FetchContent_Declare(
    nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG master
)
FetchContent_MakeAvailable(nfd)

# Find system-installed assimp (via Homebrew)
find_package(assimp REQUIRED)


# Create ImGui library
add_library(imgui_impl
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui_impl PUBLIC ${imgui_SOURCE_DIR})

# Create rlImGui library
add_library(rlimgui_lib
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)
target_include_directories(rlimgui_lib PUBLIC ${rlimgui_SOURCE_DIR})
target_link_libraries(rlimgui_lib PUBLIC raylib imgui_impl)

# Add source files
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Application.cpp
    src/model/Model.cpp
    src/model/ModelLoader.cpp
    src/model/ModelRenderer.cpp
    src/ui/MainMenuBar.cpp
    src/ui/SceneWindow.cpp
    src/ui/InspectorWindow.cpp
    src/ui/HierarchyWindow.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${raylib_SOURCE_DIR}/src
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
)

# Link libraries (order matters for static linking)
target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    rlimgui_lib
    imgui_impl
    assimp::assimp
    nfd
)

# Platform-specific settings
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
endif()

# Set the output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

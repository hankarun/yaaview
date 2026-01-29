cmake_minimum_required(VERSION 3.16)
project(AAV VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Enable debug symbols for shader debugging with RenderDoc
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(GRAPHICS_API_OPENGL_DEBUG_CONTEXT)
    if(MSVC)
        add_compile_options(/Zi)
    else()
        add_compile_options(-g)
    endif()
endif()

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
set(OPENGL_DEBUG_CONTEXT ON CACHE BOOL "" FORCE)
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

# Platform-specific Assimp handling
if(WIN32)
    # On Windows, use FetchContent to get Assimp
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.4
    )
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(assimp)
elseif(APPLE)
    # On macOS, use system-installed assimp (via Homebrew)
    find_package(assimp REQUIRED)
endif()


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
    src/ui/LogWindow.cpp
    src/ui/ModelInfoWindow.cpp
    src/ui/LightWindow.cpp
    src/ui/SettingsWindow.cpp
    src/util/Logger.cpp
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

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT application
)

# Install shaders if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/shaders")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/shaders"
        DESTINATION bin
        COMPONENT application
    )
endif()

# Install HDR files if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/hdr")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/hdr"
        DESTINATION bin
        COMPONENT application
    )
endif()

# CPack configuration for NSIS installer
include(CPack)

set(CPACK_PACKAGE_NAME "AAV")
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AAV - 3D Model Viewer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AAV")

# NSIS specific settings
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_PACKAGE_NAME "AAV")
set(CPACK_NSIS_DISPLAY_NAME "AAV - 3D Model Viewer")
set(CPACK_NSIS_HELP_LINK "https://github.com/yourrepo")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/yourrepo")
set(CPACK_NSIS_CONTACT "your.email@example.com")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Create desktop shortcut
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$DESKTOP\\\\AAV.lnk' '$INSTDIR\\\\bin\\\\AAV.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$DESKTOP\\\\AAV.lnk'"
)

# Start menu shortcuts
set(CPACK_PACKAGE_EXECUTABLES "AAV" "AAV - 3D Model Viewer")
set(CPACK_NSIS_MENU_LINKS
    "bin/AAV.exe" "AAV - 3D Model Viewer"
)
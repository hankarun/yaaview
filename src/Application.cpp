#include "Application.h"
#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"
#include "nfd.hpp"
#include <iostream>

namespace AAV {

Application::Application()
    : initialized(false)
    , showImGuiDemo(false)
    , screenWidth(1280)
    , screenHeight(720)
    , selectedNode(nullptr)
{
}

Application::~Application() {
    if (initialized) {
        Shutdown();
    }
}

bool Application::Initialize(int width, int height, const char* title) {
    if (initialized) return true;
    
    screenWidth = width;
    screenHeight = height;
    
    // Initialize Raylib window
    InitWindow(screenWidth, screenHeight, title);
    SetTargetFPS(60);
    
    // Initialize rlImGui with docking enabled
    rlImGuiSetup(true);
    
    // Enable ImGui docking
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // Setup callbacks
    HandleCallbacks();
    
    // Setup hierarchy window callback
    hierarchyWindow.onNodeSelected = [this](NodeData* node) {
        selectedNode = node;
    };
    
    initialized = true;
    
    std::cout << "Application initialized successfully" << std::endl;
    std::cout << "Screen: " << screenWidth << "x" << screenHeight << std::endl;
    
    return true;
}

void Application::Run() {
    if (!initialized) {
        std::cerr << "Application not initialized!" << std::endl;
        return;
    }
    
    while (!WindowShouldClose()) {
        // Update
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{45, 45, 48, 255});
        
        // Start ImGui frame
        rlImGuiBegin();
        
        // Setup docking space
        SetupImGuiDocking();
        
        // Render UI components
        menuBar.Render();
        sceneWindow.Render(currentModel);
        hierarchyWindow.Render(currentModel);
        inspectorWindow.Render(currentModel, selectedNode);
        
        // Show ImGui demo if requested
        if (showImGuiDemo) {
            ImGui::ShowDemoWindow(&showImGuiDemo);
        }
        
        // End ImGui frame
        rlImGuiEnd();
        
        EndDrawing();
    }
}

void Application::Shutdown() {
    if (!initialized) return;
    
    sceneWindow.Cleanup();
    
    rlImGuiShutdown();
    CloseWindow();
    
    initialized = false;
    
    std::cout << "Application shutdown complete" << std::endl;
}

void Application::SetupImGuiDocking() {
    // Create a fullscreen dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    
    // DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    
    ImGui::End();
}

void Application::HandleCallbacks() {
    // File menu callbacks
    menuBar.onOpenFile = [this]() {
        OpenModelDialog();
    };
    
    menuBar.onExit = []() {
        // Window will close on next iteration
    };
    
    // View menu callbacks
    menuBar.onToggleInspector = [this]() {
        inspectorWindow.SetVisible(!inspectorWindow.IsVisible());
    };
    
    menuBar.onToggleScene = [this]() {
        sceneWindow.SetVisible(!sceneWindow.IsVisible());
    };
    
    menuBar.onToggleHierarchy = [this]() {
        hierarchyWindow.SetVisible(!hierarchyWindow.IsVisible());
    };
    
    menuBar.onResetCamera = [this]() {
        sceneWindow.ResetCamera();
    };
    
    // Window menu callbacks
    menuBar.onLoadLayout = [this](int layout) {
        // TODO: Implement layout presets
        std::cout << "Load layout preset: " << layout << std::endl;
    };
    
    // Help menu callbacks
    menuBar.onToggleImGuiDemo = [this]() {
        showImGuiDemo = !showImGuiDemo;
    };
}

void Application::OpenModelDialog() {
    // Use native file dialog
    NFD::Guard nfdGuard;
    
    // Define file filters for 3D model formats
    nfdfilteritem_t filters[] = {
        { "3D Models", "obj,fbx,gltf,glb,dae,3ds,blend,stl,ply,x,md2,md3,md5mesh,ase,ifc" },
        { "All Files", "*" }
    };
    
    NFD::UniquePath outPath;
    nfdresult_t result = NFD::OpenDialog(outPath, filters, 2);
    
    if (result == NFD_OKAY) {
        std::string filePath(outPath.get());
        LoadModelFile(filePath);
    } else if (result == NFD_CANCEL) {
        std::cout << "User cancelled file dialog" << std::endl;
    } else {
        std::cerr << "Error opening file dialog: " << NFD::GetError() << std::endl;
    }
}

void Application::LoadModelFile(const std::string& filePath) {
    std::cout << "Loading model: " << filePath << std::endl;
    
    auto model = modelLoader.LoadModel(filePath);
    
    if (model) {
        currentModel = model;
        // Frame the model in the scene view
        sceneWindow.FrameModel(currentModel);
        std::cout << "Model loaded successfully!" << std::endl;
    } else {
        std::cerr << "Failed to load model: " << modelLoader.GetLastError() << std::endl;
    }
}

} // namespace AAV

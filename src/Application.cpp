#include "Application.h"
#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "nfd.hpp"
#include "util/Logger.h"
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
    
    Logger::Info("Application initialized successfully");
    Logger::Info("Screen: " + std::to_string(screenWidth) + "x" + std::to_string(screenHeight));
    
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
        logWindow.Render();
        modelInfoWindow.Render(currentModel);
        lightWindow.Render(&sceneWindow);
        settingsWindow.Render(&sceneWindow);
        
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
    
    Logger::Info("Application shutdown complete");
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
    
    menuBar.onToggleLog = [this]() {
        logWindow.SetVisible(!logWindow.IsVisible());
    };
    
    menuBar.onToggleModelInfo = [this]() {
        modelInfoWindow.SetVisible(!modelInfoWindow.IsVisible());
    };
    
    menuBar.onToggleLight = [this]() {
        lightWindow.SetVisible(!lightWindow.IsVisible());
    };
    
    menuBar.onToggleSettings = [this]() {
        settingsWindow.Open();
    };
    
    menuBar.onResetCamera = [this]() {
        sceneWindow.ResetCamera();
    };
    
    // Window menu callbacks
    menuBar.onLoadLayout = [this](int layout) {
        LoadLayoutPreset(layout);
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
    Logger::Info("Loading model: " + filePath);
    
    auto model = modelLoader.LoadModel(filePath);
    
    if (model) {
        currentModel = model;
        // Frame the model in the scene view
        sceneWindow.FrameModel(currentModel);
        Logger::Info("Model loaded successfully!");
    } else {
        Logger::Error("Failed to load model: " + modelLoader.GetLastError());
    }
}

void Application::SetupDefaultLayout() {
    // Setup default layout on first run
    static bool firstTime = true;
    if (!firstTime) return;
    firstTime = false;
    
    LoadLayoutPreset(0); // Load classic layout by default
}

void Application::LoadLayoutPreset(int presetIndex) {
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    
    // Clear existing layout
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);
    
    // Create dock IDs for different areas
    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_left_id;
    ImGuiID dock_right_id;
    ImGuiID dock_bottom_id;
    ImGuiID dock_left_top;
    ImGuiID dock_left_middle;
    ImGuiID dock_left_bottom;
    
    switch (presetIndex) {
        case 0: // Classic Layout
            // Split left sidebar
            dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.23f, nullptr, &dock_main_id);
            
            // Split bottom for log
            dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.35f, nullptr, &dock_main_id);
            
            // Split left sidebar vertically
            dock_left_top = ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Up, 0.33f, nullptr, &dock_left_id);
            dock_left_middle = ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Up, 0.5f, nullptr, &dock_left_bottom);
            
            // Dock windows
            ImGui::DockBuilderDockWindow("Hierarchy", dock_left_top);
            ImGui::DockBuilderDockWindow("Inspector", dock_left_middle);
            ImGui::DockBuilderDockWindow("Model Info", dock_left_bottom);
            ImGui::DockBuilderDockWindow("Scene", dock_main_id);
            ImGui::DockBuilderDockWindow("Log", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Lighting", dock_left_bottom);
            ImGui::DockBuilderDockWindow("Settings", dock_left_bottom);
            break;
            
        case 1: // Wide Inspector Layout
            // Split left sidebar (wider)
            dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.30f, nullptr, &dock_main_id);
            
            // Split right sidebar
            dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
            
            // Split bottom for log
            dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);
            
            // Split left sidebar
            dock_left_top = ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Up, 0.4f, nullptr, &dock_left_id);
            
            // Dock windows
            ImGui::DockBuilderDockWindow("Hierarchy", dock_left_top);
            ImGui::DockBuilderDockWindow("Inspector", dock_left_id);
            ImGui::DockBuilderDockWindow("Model Info", dock_right_id);
            ImGui::DockBuilderDockWindow("Lighting", dock_right_id);
            ImGui::DockBuilderDockWindow("Settings", dock_right_id);
            ImGui::DockBuilderDockWindow("Scene", dock_main_id);
            ImGui::DockBuilderDockWindow("Log", dock_bottom_id);
            break;
            
        case 2: // Full Scene Layout
            // Small left sidebar
            dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.15f, nullptr, &dock_main_id);
            
            // Small right sidebar
            dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.18f, nullptr, &dock_main_id);
            
            // Very small bottom for log
            dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, nullptr, &dock_main_id);
            
            // Split left for hierarchy stacking
            dock_left_top = ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Up, 0.5f, nullptr, &dock_left_id);
            
            // Dock windows - Scene maximized
            ImGui::DockBuilderDockWindow("Scene", dock_main_id);
            ImGui::DockBuilderDockWindow("Hierarchy", dock_left_top);
            ImGui::DockBuilderDockWindow("Model Info", dock_left_id);
            ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
            ImGui::DockBuilderDockWindow("Lighting", dock_right_id);
            ImGui::DockBuilderDockWindow("Settings", dock_right_id);
            ImGui::DockBuilderDockWindow("Log", dock_bottom_id);
            break;
    }
    
    ImGui::DockBuilderFinish(dockspace_id);
    
    Logger::Info("Loaded layout preset: " + std::to_string(presetIndex));
}

} // namespace AAV

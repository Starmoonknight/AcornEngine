#pragma once


// Project headers
#include "AcornMath.h"
#include "assets/ModelCache.h" 
#include "rendering/Camera.h"
#include "rendering/GpuModel.h"
#include "rendering/Renderer.h"
#include "rendering/RenderInstances.h"
#include "rendering/Texture.h"
#include "Palette.h"


// Standard library


// Third-party libraries
#include <glfw/glfw3.h>
#include <glm/glm.hpp>     
#include "stb_image.h"

namespace AcornData
{
    struct SceneDescription;
}

struct Scene;
struct SceneObject;
struct SceneCamera;

namespace Demo
{
    
    
    struct GeometryRuntimeValues_Inputs
    {
        bool bPreviousPress_Space = false;
        
        // Scale
        bool bPreviousPress_Q = false;
        bool bPreviousPress_W = false;
        bool bPreviousPress_E = false;
        
        // Rotation
        bool bPreviousPress_A = false;
        bool bPreviousPress_S = false;
        bool bPreviousPress_D = false;
        
        // Location
        bool bPreviousPress_Z = false;
        bool bPreviousPress_X = false;
        bool bPreviousPress_C = false;
    };
    
    
    struct GeometryRuntimeValues_Transforms
    {   
        // Scale
        bool bScaleX = false;
        bool bScaleY = false;
        bool bScaleZ = false;
        
        glm::vec3 scale = glm::vec3(1.0f);
        
        bool bGrowingX = true;
        bool bGrowingY = true;
        bool bGrowingZ = true;
    
        // Rotation
        bool bRotateX = false;
        bool bRotateY = false;
        bool bRotateZ = false;
    
        glm::vec3 rotationDegrees = glm::vec3 (0.0f);
    
        // Location
        bool bTranslateX = false;
        bool bTranslateY = false;
        bool bTranslateZ = false;
        
        glm::vec3 translation = glm::vec3 (0.0f);
        
        bool bPositivMovementX = true; 
        bool bPositivMovementY = true; 
        bool bPositivMovementZ = true; 
    };
    
    
    
    
    void HandleRuntimeInputs(
        GLFWwindow* window, 
        GeometryRuntimeValues_Inputs& inputs, 
        GeometryRuntimeValues_Transforms& values);
    
    
    
    void Update_Transforms(GeometryRuntimeValues_Transforms& values, float deltaTime);
    
    
    
    
    
    
    void UpdateModelPair(
        GLFWwindow* window,
        GeometryRuntimeValues_Inputs& inputs,
        GeometryRuntimeValues_Transforms& animation,
        AcornData::ModelInstance& left,
        AcornData::ModelInstance& right,
        float deltaTime); 
    
    
    
    
    void SetCameraOrbitPose(
        Rendering::CameraData& camera,
        const glm::vec3& target,
        float angleDegrees,
        glm::vec3 startingOffset);
    
    
    
    void UpdateOrbitingCamera(SceneCamera& camera, float deltaTime);
    
    
    void UpdateCameraPos(
        GLFWwindow* window, 
        Rendering::CameraData& cameraData, 
        float cameraSpeed, 
        float deltaTime);
    
    
    
    
    
    
    // Create player bodies using shared model assets and texture records.
    // Body appearance is converted to a private material after all initial
    // player-specific settings have been assigned.
    bool CreatePlayers(
        Scene& scene,
        AcornData::TextureId texture,
        const std::string& filePath);
    
    
    
    std::array<AcornData::SceneDescription, 3> MakeDemoScenes(); 
    
    
    
    // Add importer-comparison placements using a shared texture reference.
    // The scene owns the placements; application services own shared assets.
    void PopulateComparison_Backpack(
        Scene& loaded,
        AcornData::TextureId texture);
    
    void PopulateComparison_Teapot(
        Scene& loaded,
        AcornData::TextureId texture);
    
    
    void PopulateComparison_Ipshead(
        Scene& loaded,
        AcornData::TextureId texture);
    
    
    void PopulateComparison_Cube(
        Scene& loaded,
        AcornData::TextureId texture);
    
    
    void PopulateComparison_Pine(
        Scene& loaded,
        AcornData::TextureId texture);
    
    
    void PopulateComparison_SportsCar(
        Scene& loaded,
        AcornData::TextureId texture);
    
    
    
    
    // --------------- Making Scenes --------------------
    
    // Structs to make a scene, that will then be used to save a scene as a file, 
    // that can then be used to load scenes instead of using these hardcode structs. 
    
    
    struct ForestScene
    {
        static AcornData::SceneDescription Build();
    };

    struct TeapotScene
    {
        static AcornData::SceneDescription Build();
    };
    
    struct PillarScene
    {
        static AcornData::SceneDescription Build();
    };
    
}


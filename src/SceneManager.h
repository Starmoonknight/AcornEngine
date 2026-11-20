#pragma once



// Project headers
#include "rendering/Camera.h"


// Standard library


// Third-party libraries
#include <glm/glm.hpp>     


struct FramebufferSize;
class ShaderProgram;


namespace AcornData
{
    struct RenderModelInstance;
}







struct FramebufferSize
{
    int width = 0;
    int height = 0;
};



enum SplitScreenSettings
{
    VerticalSplit,
    HorizontalSplit,
};


struct Sun
{
    glm::vec3 lightColor{1.0f};
    float lightStrength = 1.0f;
    float sunAngleDegrees = 0.0f;
};


struct SceneSettings
{
    Sun sun{}; 
    
    SplitScreenSettings splitScreen = VerticalSplit;
};


struct SceneCamera
{
    bool enabled = true; 
    Rendering::CameraData camera;

    glm::vec3 backgroundColor{0.0f};
    
    glm::vec3 target{0.0f};
    glm::vec3 startingOffset{4.0f, 5.0f, 7.0f};
    float orbitAngleDegrees = 0.0f;
};



struct Scene
{
    int SceneID = -1;   // -1 is invalid, 0 default, and new scenes should be added with +1 
    SceneSettings settings;
    
    std::vector<AcornData::RenderModelInstance*> models;
    std::vector<SceneCamera> cameras;
};




void AddSceneModel(Scene& scene, AcornData::RenderModelInstance* model); 
void RemoveSceneModel(Scene& scene, AcornData::RenderModelInstance* model);


void ConfigureSceneViewports(Scene& scene, const FramebufferSize& framebuffer);




// Update once.
void UpdateScene(Scene& scene, float deltaTime);

// Draw the resulting state through each camera.
void DrawScene(const Scene& scene, const ShaderProgram& shader);




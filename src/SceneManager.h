#pragma once



// Project headers
#include "rendering/Camera.h"
#include "Palette.h"


// Standard library
#include <vector>


// Third-party libraries
#include <glm/glm.hpp>     


struct FramebufferSize;
class ShaderProgram;


namespace AcornData
{
    struct RenderModelInstance;
}


namespace Demo
{
    
    enum class SplitScreenSettings_Extra_Carousel
    {
        // Useless, but fun to try to create: 
        CarouselVerticalSplit,      // Moving over screen sideways Left to Right
        CarouselHorizontalSplit,    // Moving over screen from Up to Down screen
        CarouselGridSplit           // Circling clockwise 
    };
    
}





struct FramebufferSize
{
    int width = 0;
    int height = 0;
};



enum SplitScreenSettings
{
    VerticalSplit,              // Vertical dividing lines: views arranged left to right.
    HorizontalSplit,            // Horizontal dividing lines: views arranged bottom to top.
    GridSplit,                  // Discord or Teams similar layout 
};



struct Sun
{
    glm::vec3 lightColor{1.0f};
    float lightStrength = 1.0f;
    float sunAngleDegrees = 0.0f;
};


struct SceneSettings
{
    glm::vec3 defaultBackgroundColor{Palette::lightBlue};
    
    Sun sun{}; 
    
    SplitScreenSettings splitScreen = VerticalSplit;
};


struct SceneCamera
{
    int ID = -1;   // -1 is invalid, 0 default, and new scenes should be added with +1 
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
// Note:  Currently will have to re-calculate the model matrix for each camera instead of once and then applied to each camera draw. 
void DrawScene(const Scene& scene, const ShaderProgram& shader);




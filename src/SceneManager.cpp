#include "SceneManager.h"


// Project headers
#include "rendering/Renderer.h"

// Standard library
#include <algorithm>



namespace 
{
    
    constexpr float nearPlane = 0.1f;
    constexpr float farPlane = 100.0f;
    constexpr float FOV = 70.0f;
    
    constexpr glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    
    
    
    
    
}




void AddSceneModel(Scene& scene, AcornData::RenderModelInstance* model)
{
    if (model == nullptr)
        return;
    
    // searches the list for the pointer value supplied as model, 
    // if this model’s address is not already registered, append it.
    if (std::find(scene.models.begin(), scene.models.end(), model)
        == scene.models.end())                          // if there is no match, it returns end()
    {
        scene.models.push_back(model);
    }
}


void RemoveSceneModel(Scene& scene, AcornData::RenderModelInstance* model)
{
    // Removes every list entry whose pointer value equals model.
    // Only erases the stored pointer, object cleanup is handled by owner 
    std::erase(scene.models, model);
}



void ConfigureSceneViewports(Scene& scene, const FramebufferSize& framebuffer)
{
    assert(scene.cameras.size() == 1);

    scene.cameras.front().camera.SetViewport(
        0, 0, framebuffer.width, framebuffer.height);
}




void UpdateScene(Scene& scene, float deltaTime)
{
    
    
    
    
    
}



void DrawScene(const Scene& scene, const ShaderProgram& shader)
{
    for (const SceneCamera& sceneCamera : scene.cameras)
    {
        if (!sceneCamera.enabled ||
            !sceneCamera.camera.HasValidViewport())
        {
            continue;
        }
        
        const Rendering::Viewport& viewport = sceneCamera.camera.viewport;
        
        glViewport(
            viewport.x,
            viewport.y,
            viewport.width,
            viewport.height);
        
        const glm::vec3& background = sceneCamera.backgroundColor;
        
        glClearColor(background.r, background.g, background.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        
        const Rendering::CameraMatrices cameraMatrices = sceneCamera.camera.GetMatrices(); 
        
        
        for (const AcornData::RenderModelInstance* model : scene.models)
        {
            if (model != nullptr)
            {
                Rendering::DrawModel(shader, cameraMatrices, *model);
            }
        }
    }
}

#include "SceneManager.h"


// Project headers
#include "rendering/Renderer.h"

// Standard library
#include <algorithm>
#include <cmath>



namespace 
{
    
    constexpr float nearPlane = 0.1f;
    constexpr float farPlane = 100.0f;
    constexpr float FOV = 70.0f;
    
    constexpr glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    
    // For Vertical or Horizontal split screen 
    void ConfigureStripViewports(
        Scene& scene,
        const FramebufferSize& framebuffer,
        int enabledCount,
        bool sideBySide)
    {
            
        const int availablePixels = sideBySide
            ? framebuffer.width
            : framebuffer.height;
    
        const int baseSize = availablePixels / enabledCount;
        const int extraPixels = availablePixels % enabledCount;
    
        int enabledIndex = 0;
        int offset = 0;
    
        for (SceneCamera& sceneCamera : scene.cameras)
        {
            if (!sceneCamera.enabled)
                continue;

            const int size =
                baseSize + (enabledIndex < extraPixels ? 1 : 0);

            if (sideBySide)
            {
                sceneCamera.camera.SetViewport(
                    offset, 0,
                    size, framebuffer.height);
            }
            else
            {
                sceneCamera.camera.SetViewport(
                    0, offset,
                    framebuffer.width, size);
            }

            offset += size;
            ++enabledIndex;
        }    
    }

    
    void ConfigureGridViewports(
    Scene& scene,
    const FramebufferSize& framebuffer,
    int enabledCount)
    {
        // Prioritizes a roughly square number of cells in columns and rows 
        
        const int columns = static_cast<int>(
            std::ceil(std::sqrt(static_cast<double>(enabledCount))));

        // The integer formula for rows rounds division upward. Ordinary integer 
        // division 3 / 2 would produce 1, but two rows are needed in that scenario.
        const int rows = (enabledCount + columns - 1) / columns;

        const int tileWidth = framebuffer.width / columns;

        int enabledIndex = 0;

        for (SceneCamera& sceneCamera : scene.cameras)
        {
            if (!sceneCamera.enabled)
                continue;

            const int row = enabledIndex / columns;
            const int column = enabledIndex % columns;

            const int camerasInRow = std::min(
                columns,
                enabledCount - row * columns);
            
            const int rowWidth = camerasInRow * tileWidth;
            const int rowStartX = (framebuffer.width - rowWidth) / 2;

            const int x = rowStartX + column * tileWidth;

            // Number rows from the top, then convert to OpenGL's
            // bottom-left framebuffer coordinates.
            const int top =
                framebuffer.height - row * framebuffer.height / rows;

            const int bottom =
                framebuffer.height - (row + 1) * framebuffer.height / rows;

            sceneCamera.camera.SetViewport(
                x,
                bottom,
                tileWidth,
                top - bottom);

            ++enabledIndex;
        }
    }
    
    
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
    
    int enabledCount = 0;
    
    for (SceneCamera& sceneCamera : scene.cameras)
    {
        // Clear any rectangle left from the previous layout.
        sceneCamera.camera.SetViewport(0, 0, 0, 0);

        // Only enabled cameras should receive space.
        if (sceneCamera.enabled)
            ++enabledCount;
    }
    
    if (enabledCount == 0 || framebuffer.width <= 0 || framebuffer.height <= 0)
    {
        return;
    }
    
    switch (scene.settings.splitScreen)
    {
    case VerticalSplit:
        ConfigureStripViewports(
            scene, framebuffer, enabledCount, true);
        break;

    case HorizontalSplit:
        ConfigureStripViewports(
            scene, framebuffer, enabledCount, false);
        break;

    case GridSplit:
        ConfigureGridViewports(
            scene, framebuffer, enabledCount);
        break;
    }    
}




void UpdateScene(Scene& scene, float deltaTime)
{
    
    
    
    
    
}



void DrawScene(const Scene& scene, const ShaderProgram& shader)
{
    // Establish a clean whole-frame background first.
    // This also gives a defined result if every camera is disabled.
    glDisable(GL_SCISSOR_TEST);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_SCISSOR_TEST);
    
    
    
    for (const SceneCamera& sceneCamera : scene.cameras)
    {
        if (!sceneCamera.enabled ||
            !sceneCamera.camera.HasValidViewport())
        {
            continue;
        }
        
        const Rendering::Viewport& viewport = sceneCamera.camera.viewport;
        
        // Map this camera's image into its rectangle.
        glViewport(
            viewport.x, viewport.y,
            viewport.width, viewport.height);
        
        // Restrict clearing and drawing to the same rectangle.
        glScissor(
            viewport.x, viewport.y,
            viewport.width, viewport.height);
        
        // Set the background color saved in this camera 
        const glm::vec3& background = sceneCamera.backgroundColor;
        glClearColor(
            background.r,
            background.g,
            background.b,
            1.0f);
        
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
    
    
    // Later rendering should not inherit the last camera's restriction.
    glDisable(GL_SCISSOR_TEST);
}

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
    
    
    
    // Divides the framebuffer into adjacent strips for enabled cameras.
    // Distributes remainder pixels so the strips cover the full dimension.
    // Called only with positive framebuffer dimensions and enabledCount > 0.
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

    
    // Arranges enabled cameras in a roughly square grid.
    // Centers an incomplete final row. Integer-width cells can leave
    // unused edge pixels, which DrawScene clears to the frame background.
    // Called only with positive framebuffer dimensions and enabledCount > 0.
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






SceneObject* Scene::AddObject(const AcornData::SceneObjectDescription& description,GLuint textureID)
{
    // Resolve the source and obtain shared geometry owned by this cache.
    const auto* geometry = modelCache.GetOrLoad(description.source);

    if (geometry == nullptr)
        return nullptr;

    // Appending creates the owning object without relocating old objects.
    SceneObject& object = placedObjects.emplace_back();

    // Keep enough source information to reconstruct it later.
    object.source = description.source;

    // Configure its runtime instance.
    object.instance.gpuModel = geometry;
    object.instance.transform = description.transform;
    object.instance.renderSettings = description.appearance;
    object.instance.textureID = textureID; 

    // Expose the instance to rendering without transferring ownership.
    RegisterRenderInstance(&object.instance);

    return &object;
}


void Scene::Clear()
{
    // Remove borrowed addresses before destroying their targets.
    renderInstances.clear();
    
    // Destroy runtime contents before releasing geometry they borrow.
    cameras.clear();
    placedObjects.clear();
    
    // Release all uploaded cached models, preserves the cache's fixed root.
    modelCache.Clear();
    
    // Forget the previous level's metadata and starting state.
    settings = AcornData::SceneSettings{};
    name.clear();
    sceneID = -1;
    playerStarts = {};
    goalPosition = glm::vec3(0.0f);
}



void Scene::RegisterRenderInstance(AcornData::RenderModelInstance* model)
{
    if (model == nullptr)
        return;
    
    
    // Compare addresses, not instance contents.
    // Different instances may share geometry and still need separate draws.
    if (std::find(renderInstances.begin(), renderInstances.end(), model)
        == renderInstances.end())                          // if there is no match, it returns end()
    {
        renderInstances.push_back(model);
    }
}


void Scene::UnregisterRenderInstance(AcornData::RenderModelInstance* model)
{
    // Removes every list entry whose pointer value equals model.
    // Only erases the stored pointer, object cleanup is handled by owner 
    std::erase(renderInstances, model);
}



void Scene::ConfigureSceneViewports(const FramebufferSize& framebuffer)
{
    
    int enabledCount = 0;
    
    for (SceneCamera& sceneCamera : cameras)
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
    
    switch (settings.splitScreen)
    {
    case AcornData::SplitScreenSettings::VerticalSplit:
        ConfigureStripViewports(
            *this, framebuffer, enabledCount, true);
        break;

    case AcornData::SplitScreenSettings::HorizontalSplit:
        ConfigureStripViewports(
            *this, framebuffer, enabledCount, false);
        break;

    case AcornData::SplitScreenSettings::GridSplit:
        ConfigureGridViewports(
            *this, framebuffer, enabledCount);
        break;
    }    
}




void Scene::UpdateScene(float deltaTime)
{
    
    
    
    
    
}



void Scene::DrawScene(const ShaderProgram& shader)
{
    // Establish a clean whole-frame background first.
    // This also gives a defined result if every camera is disabled.
    glDisable(GL_SCISSOR_TEST);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_SCISSOR_TEST);
    
    
    
    for (const SceneCamera& sceneCamera : cameras)
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
        
        
        for (const AcornData::RenderModelInstance* model : renderInstances)
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







bool LoadSceneFromDescription(
    Scene& destination, 
    const AcornData::SceneDescription& description)
{
    // Deliberate policy: fully unload the old level before loading this one.
    destination.Clear();
    
    destination.name = description.name;
    destination.sceneID = description.sceneID;
    destination.settings = description.settings;
    destination.playerStarts = description.playerStarts;
    destination.goalPosition = description.goalPosition;
    
    // Temporary viewpoints until Player objects control these cameras.
    destination.cameras.resize(description.playerStarts.size());
    
    
    for (std::size_t i = 0; i < description.playerStarts.size(); ++i)
    {
        const auto& start = description.playerStarts[i];
        SceneCamera& view = destination.cameras[i];

        // set up players camera 
        view.ID = static_cast<int>(i);
        view.enabled = true;
        view.backgroundColor =
            description.settings.defaultBackgroundColor;

        view.camera.SetPosition(start.position);
        view.camera.SetRotationDegrees(start.rotationDegrees);
    }
    
    // Currently level fails if objects don't load. 
    // Maybe chang to warning later, allow scenes with missing models? 
    for (const auto& objectDescription : description.objects)
    {
        if (destination.AddObject(objectDescription) == nullptr)
        {
            // Do not leave a partially constructed level active.
            destination.Clear();
            return false;
        }
    }
    
    return true;
    
}























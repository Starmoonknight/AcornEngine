#include "CarouselDemo.h"


// Project headers
#include "SceneManager.h"
#include "rendering/Renderer.h"

// Standard library
#include <algorithm>
#include <cmath>




namespace
{
    void DrawCarouselPiece(
        const Scene& scene,
        const SceneCamera& sceneCamera,
        const ShaderProgram& shader,
        const Rendering::CameraMatrices& matrices,
        const FramebufferSize& framebuffer,
        int x,
        int y)
    {
        const Rendering::Viewport& original =
            sceneCamera.camera.viewport;

        // Find the intersection with the framebuffer.
        const int left = std::max(0, x);
        const int bottom = std::max(0, y);

        const int right = std::min(
            framebuffer.width,
            x + original.width);

        const int top = std::min(
            framebuffer.height,
            y + original.height);

        if (right <= left || top <= bottom)
            return;

        // Keep the full image dimensions, even if partly offscreen.
        glViewport(x, y, original.width, original.height);

        // Allow writes only to the visible part.
        glScissor(
            left,
            bottom,
            right - left,
            top - bottom);

        const glm::vec3& background = sceneCamera.backgroundColor;

        glClearColor(
            background.r,
            background.g,
            background.b,
            1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (const AcornData::RenderModelInstance* model : scene.models)
        {
            if (model != nullptr)
                Rendering::DrawModel(shader, matrices, *model);
        }
    }
}





namespace Demo
{
    
    void UpdateCarousel(CarouselSettings& settings, float deltaTime)
    {
        if (settings.secondsPerLoop <= 0.0f || deltaTime <= 0.0f)
            return;

        settings.phase = std::fmod(
            settings.phase + deltaTime / settings.secondsPerLoop,
            1.0f);
    }

    
    void DrawCarouselScene(
        const Scene& scene,
        const ShaderProgram& shader,
        const FramebufferSize& framebuffer,
        const CarouselSettings& settings)
    {
        if (framebuffer.width <= 0 || framebuffer.height <= 0)
            return;

        // Grid animation is not implemented in this first step.
        // Display the normal grid as its explicit temporary fallback.
        if (settings.layout ==
            SplitScreenSettings_Extra_Carousel::CarouselGridSplit)
        {
            Scene preview = scene;
            preview.settings.splitScreen = GridSplit;

            ConfigureSceneViewports(preview, framebuffer);
            DrawScene(preview, shader);
            return;
        }

        const bool moveSideways =
            settings.layout ==
            SplitScreenSettings_Extra_Carousel::CarouselVerticalSplit;

        // Copy camera/layout data so the experiment does not overwrite
        // the real scene's normal viewport configuration.
        Scene preview = scene;

        preview.settings.splitScreen =
            moveSideways ? VerticalSplit : HorizontalSplit;

        ConfigureSceneViewports(preview, framebuffer);

        const int travelDistance = moveSideways
            ? framebuffer.width
            : framebuffer.height;

        const int displacement = static_cast<int>(
            settings.phase * travelDistance);

        glDisable(GL_SCISSOR_TEST);

        const glm::vec3& background =
            scene.settings.defaultBackgroundColor;

        glClearColor(
            background.r,
            background.g,
            background.b,
            1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_SCISSOR_TEST);

        for (const SceneCamera& camera : preview.cameras)
        {
            if (!camera.enabled || !camera.camera.HasValidViewport())
                continue;

            const Rendering::CameraMatrices matrices =
                camera.camera.GetMatrices();

            const Rendering::Viewport& viewport = camera.camera.viewport;

            for (int copy = -1; copy <= 1; ++copy)
            {
                const int wrapOffset = copy * travelDistance;

                const int x = moveSideways
                    ? viewport.x + displacement + wrapOffset
                    : viewport.x;

                const int y = moveSideways
                    ? viewport.y
                    : viewport.y - displacement + wrapOffset;

                DrawCarouselPiece(
                    preview,
                    camera,
                    shader,
                    matrices,
                    framebuffer,
                    x,
                    y);
            }
        }

        glDisable(GL_SCISSOR_TEST);
    }
    
}

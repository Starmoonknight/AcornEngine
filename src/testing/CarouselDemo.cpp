#include "CarouselDemo.h"


// Project headers
#include "SceneManager.h"
#include "rendering/Renderer.h"

// Standard library
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <vector>




namespace
{
    
    // One original grid position and the camera that occupied it.
    //
    // We save the viewport by value because applying the shuffle changes
    // camera viewports. Reading directly from already-modified cameras
    // could otherwise overwrite information needed by later assignments.
    struct GridCarouselSlot
    {
        // Index into preview.cameras, not the camera's user-defined ID.
        std::size_t cameraIndex;
        
        // Original position and dimensions assigned by the normal grid.
        Rendering::Viewport viewport;
        
        // Angular position around the framebuffer center.
        // Measured in radians, clockwise from upward, in [0, 2*pi).
        float clockwiseAngle;
    };
    
    
    
    // Comparison function used by stable_sort.
    //
    // Returning true means "left should appear before right".
    // Sorting ascending angles gives a clockwise sequence of grid slots.
    bool ComesEarlierClockwise(
    const GridCarouselSlot& left,
    const GridCarouselSlot& right)
    {
        return left.clockwiseAngle < right.clockwiseAngle;
    }
    
    
    
    // Reassigns camera viewports to existing grid positions.
    //
    // Required setup:
    //   ConfigureSceneViewports has already assigned a normal GridSplit.
    //   phase is in [0, 1).
    //
    // This changes only the temporary preview's viewport assignments.
    // Camera poses, backgrounds, and model transforms are unchanged.
    //
    // The route is based on angle around the framebuffer center.
    // For large grids, inner and outer positions participate in the same
    // angular ordering; this is not a separate carousel for each ring.
    void ApplyGridCarousel(Scene& preview, float phase,
                       const FramebufferSize& framebuffer)
    {
        
        // Framebuffer coordinates use a bottom-left origin.
        // Half the width/height gives the centre in those coordinates.
        const float centreX = framebuffer.width * 0.5f;
        const float centreY = framebuffer.height * 0.5f;

        // acos(-1) is pi radians; twice that is one complete turn.
        // atan2 below also returns radians, so the units match.
        const float fullTurn = 2.0f * std::acos(-1.0f);

        std::vector<GridCarouselSlot> slots;

        // Collect the original grid positions before modifying any camera.
        for (std::size_t index = 0;
             index < preview.cameras.size();
             ++index)
        {
            const SceneCamera& camera = preview.cameras[index];

            // Disabled cameras have no place in the route.
            // Zero-area viewports can occur with very small framebuffers.
            if (!camera.enabled || !camera.camera.HasValidViewport())
                continue;

            const Rendering::Viewport viewport = camera.camera.viewport;

            // A viewport stores its lower-left corner.
            // Adding half its dimensions finds the center of its tile.
            const float tileCentreX =
                viewport.x + viewport.width * 0.5f;

            const float tileCentreY =
                viewport.y + viewport.height * 0.5f;

            // Direction from framebuffer center to tile center.
            //
            // Positive dx: tile is to the right.
            // Positive dy: tile is above the center.
            const float dx = tileCentreX - centreX;
            const float dy = tileCentreY - centreY;

            // A tile exactly at the centre has no meaningful angular
            // position around that centre, so leave it stationary.
            //
            // The half-pixel tolerance accounts for integer rounding in
            // grid layout, which can slightly offset a nominal centre tile.
            if (std::abs(dx) <= 0.5f && std::abs(dy) <= 0.5f)
                continue;

            // The usual atan2(y, x) measures from the positive X axis.
            //
            // Using atan2(dx, dy) intentionally changes the reference:
            //   up    -> 0 degrees
            //   right -> 90 degrees
            //   down  -> 180 degrees
            //   left  -> -90 degrees, converted to 270 below
            //
            // This gives clockwise angles in our bottom-left coordinate
            // system. It sorts screen positions; it does not rotate cameras.
            float angle = std::atan2(dx, dy);

            // atan2 can return a negative angle.
            // Adding a full turn preserves its direction while placing
            // it in the same non-negative range as the other angles.
            if (angle < 0.0f)
                angle += fullTurn;

            slots.push_back(GridCarouselSlot{
                index,
                viewport,
                angle
            });
        }

        // No exchange is possible with zero or one moving position.
        // This also avoids division/remainder operations with a zero count.
        if (slots.size() < 2)
            return;

        // Order the saved positions clockwise.
        //
        // stable_sort preserves the original order for equal angles.
        // Equal angles can occur when inner and outer tiles line up.
        std::stable_sort(
            slots.begin(),
            slots.end(),
            ComesEarlierClockwise);

        const std::size_t slotCount = slots.size();

        // Convert continuous cycle progress into a discrete position shift.
        //
        // Example with four slots:
        //   phase 0.00 .. <0.25 -> step 0
        //   phase 0.25 .. <0.50 -> step 1
        //   phase 0.50 .. <0.75 -> step 2
        //   phase 0.75 .. <1.00 -> step 3
        //
        // Casting a non-negative value to size_t drops the fraction.
        // The remainder also wraps a rounded boundary result back to zero.
        const std::size_t step =
            static_cast<std::size_t>(phase * slotCount) % slotCount;

        for (std::size_t source = 0; source < slotCount; ++source)
        {
            // Advance through the clockwise list.
            //
            // % wraps the destination after the last slot:
            // with four slots, (3 + 1) % 4 is 0.
            //
            // Every source uses the same shift, so each destination is
            // assigned exactly once. Cameras cannot share a destination
            // as a result of this permutation.
            const std::size_t destination =
                (source + step) % slotCount;

            const Rendering::Viewport& target =
                slots[destination].viewport;

            // Move the original occupant of the source slot.
            // cameraIndex still refers to its position in preview.cameras;
            // sorting slots did not reorder the camera vector.
            SceneCamera& camera =
                preview.cameras[slots[source].cameraIndex];

            // Change screen placement, not the camera's world-space pose.
            // Copy dimensions too: projection will use the destination
            // viewport's aspect ratio when DrawScene requests matrices.
            camera.camera.SetViewport(
                target.x,
                target.y,
                target.width,
                target.height);
        }
    }
    
    
    
    
    
    // Draws one translated copy of a camera image.
    //
    // x and y are the lower-left corner of the FULL translated viewport.
    // They may be negative or beyond the framebuffer edge.
    //
    // The caller must enable GL_SCISSOR_TEST before calling this function.
    // The supplied matrices describe the original full-size camera image.
    //
    // The viewport controls image placement and scaling.
    // The scissor rectangle crops that image to its visible portion.
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

        // Intersect the translated rectangle with the framebuffer:
        //
        // translated horizontal range: [x, x + width)
        // framebuffer horizontal range: [0, framebuffer.width)
        //
        // The intersection starts at the larger left boundary and ends
        // at the smaller right boundary. The same applies vertically.
        //
        // Example: x=-100 and width=400 in an 800-pixel framebuffer:
        //   left  = max(0, -100) = 0
        //   right = min(800, 300) = 300
        // Therefore 300 pixels of this copy are visible.
        const int left = std::max(0, x);
        const int bottom = std::max(0, y);

        const int right = std::min(
            framebuffer.width,
            x + original.width);

        const int top = std::min(
            framebuffer.height,
            y + original.height);

        // An empty or reversed intersection means the copy has no visible
        // pixels. Do not clear or draw anything for this copy.
        if (right <= left || top <= bottom)
            return;

        // Preserve the full image dimensions.
        //
        // Shrinking this viewport to the visible portion would squeeze
        // the entire camera image into that smaller area.
        // Instead, move the full image partly offscreen and crop it.
        glViewport(x, y, original.width, original.height);

        // Restrict framebuffer writes to the visible intersection.
        //
        // Unlike the viewport, the enabled scissor test also restricts
        // glClear. This prevents one piece's clear from erasing other views.
        glScissor(
            left,
            bottom,
            right - left,
            top - bottom);

        const glm::vec3& background = sceneCamera.backgroundColor;

        // Select the clear colour, then clear only this piece's area.
        // Depth must be cleared too so this camera starts with fresh
        // depth information for its visible pixels.
        glClearColor(
            background.r,
            background.g,
            background.b,
            1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the same scene state through this camera.
        //
        // DrawModel handles per-instance visibility.
        // No model or camera animation is advanced here: a camera may
        // need two visible copies, but the world still updates only once.
        for (const AcornData::RenderModelInstance* model : scene.renderInstances)
        {
            // sceneCamera.hiddenInstance is temp-player code, to hide player from its own camera 
            if (model != nullptr && model != sceneCamera.hiddenInstance)
                Rendering::DrawModel(shader, matrices, *model);
        }
    }
    
    
    
}





namespace Demo
{
    
    void UpdateCarousel(CarouselSettings& settings, float deltaTime)
    {
        // A non-positive loop duration cannot be used as a divisor.
        // Non-positive elapsed time should not advance this animation.
        //
        // This is a simple demo guard, not validation of every possible
        // invalid floating-point value. Timing inputs must be finite.
        if (settings.secondsPerLoop <= 0.0f || deltaTime <= 0.0f)
            return;

        // Convert elapsed seconds into a fraction of one cycle.
        //
        // Example:
        //   deltaTime      = 0.016 seconds
        //   secondsPerLoop = 8 seconds
        //   phase change   = 0.016 / 8 = 0.002
        //
        // fmod(..., 1) removes completed cycles:
        //   1.002 becomes 0.002
        //
        // The stored phase is independent of framebuffer dimensions.
        // Resizing changes the pixel displacement, not the cycle duration.
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
        
        // No drawable area, for example while the window is minimized.
        // Avoid configuring or rendering zero-size layouts.
        if (framebuffer.width <= 0 || framebuffer.height <= 0)
            return;
        
        // Grid mode changes assignments between stationary grid positions.
        // It does not need the offscreen copies used by strip scrolling.
        if (settings.layout ==
            ESplitScreenSettings_Extra_Carousel::CarouselGridSplit)
        {
            
            // Temporary preview: owns camera copies and an empty model cache.
            // Render registrations borrow the original scene's live instances.
            // Do not call AddObject on this preview or retain it after this draw.
            Scene preview{std::filesystem::current_path()};
            preview.cameras = scene.cameras;
            preview.settings = scene.settings;
            preview.renderInstances = scene.renderInstances;
            preview.settings.splitScreen = AcornData::ESplitScreenSettings::GridSplit;

            // Rebuild the original grid every frame, then apply the shift
            // derived from phase. This avoids accumulating permutations.
            preview.ConfigureSceneViewports(framebuffer);
            ApplyGridCarousel(preview, settings.phase, framebuffer);

            // Each assigned rectangle is a complete camera view, so the
            // normal scene renderer can perform clearing and drawing..
            preview.DrawScene(shader);
            return;
        }

        // For the two remaining valid enum values:
        //   true  -> side-by-side strips traveling right
        //   false -> stacked strips traveling down
        const bool moveSideways =
            settings.layout ==
            ESplitScreenSettings_Extra_Carousel::CarouselVerticalSplit;

        // Temporary preview: owns camera copies and an empty model cache.
        // Render registrations borrow the original scene's live instances.
        // Do not call AddObject on this preview or retain it after this draw.
        Scene preview{std::filesystem::current_path()};
        preview.cameras = scene.cameras;
        preview.settings = scene.settings;
        preview.renderInstances = scene.renderInstances;

        preview.settings.splitScreen =
            moveSideways ? AcornData::ESplitScreenSettings::VerticalSplit : AcornData::ESplitScreenSettings::HorizontalSplit;

        // Reuse the normal strip layout to obtain sizes and starting
        // positions. Enabled cameras divide the available area.
        preview.ConfigureSceneViewports(framebuffer);

        // One loop travels one whole framebuffer dimension.
        // This is also the spacing between repeated copies.
        const int travelDistance = moveSideways
            ? framebuffer.width
            : framebuffer.height;

        // Convert cycle progress into a whole-pixel displacement.
        //
        // Example: phase=0.25 in an 800-pixel-wide framebuffer -> 200 pixels.
        // Casting drops the fraction, so motion advances in pixel steps.
        const int displacement = static_cast<int>(
            settings.phase * travelDistance);

        // Begin with an unrestricted full-frame clear.
        // Without disabling scissor testing first, a previous scissor
        // rectangle could restrict the clear to only part of the window.
        glDisable(GL_SCISSOR_TEST);

        const glm::vec3& background =
            scene.settings.defaultBackgroundColor;
        
        glClearColor(
            background.r,
            background.g,
            background.b,
            1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Every piece below supplies its own scissor rectangle.
        // This makes its background/depth clear local to that piece.
        glEnable(GL_SCISSOR_TEST);

        for (const SceneCamera& camera : preview.cameras)
        {
            if (!camera.enabled || !camera.camera.HasValidViewport())
                continue;

            // Calculate matrices once for this camera's full image.
            //
            // All wrapped copies use identical matrices. Only the image's
            // destination rectangle changes, not its viewpoint or aspect.
            const Rendering::CameraMatrices matrices =
                camera.camera.GetMatrices();

            const Rendering::Viewport& viewport = camera.camera.viewport;

            // Try the translated image and one repeated copy on each side.
            //
            // With phase in [0, 1), travel is less than one framebuffer
            // width/height. These three placements cover the wrapped image.
            // DrawCarouselPiece skips copies completely outside the frame.
            //
            // Example: framebuffer width=800, tile width=400, translated x=700:
            //   copy  0 starts at  700 -> visible on the right
            //   copy -1 starts at -100 -> continuation visible on the left
            //   copy +1 starts at 1500 -> outside; skipped
            for (int copy = -1; copy <= 1; ++copy)
            {
                const int wrapOffset = copy * travelDistance;

                // Horizontal travel increases X: left to right.
                // In vertical travel, X stays at its original strip position.
                const int x = moveSideways
                    ? viewport.x + displacement + wrapOffset
                    : viewport.x;

                // Framebuffer Y increases upward, so subtract displacement
                // to travel downward. Repeated copies above/below provide
                // the continuation through the opposite edge.
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

        // Stop restricting later rendering to the last visible piece.
        //
        // This establishes "scissor disabled" as our exit state; it does
        // not restore whichever scissor state existed before this function.
        glDisable(GL_SCISSOR_TEST);
    }
    
}

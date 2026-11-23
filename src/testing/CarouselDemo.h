#pragma once


struct Scene;
struct FramebufferSize;
class ShaderProgram;





namespace Demo
{
    
    // Experimental layouts, deliberately separate from the normal
    // SplitScreenSettings enum.
    //
    // Selecting one of these does not change the normal scene layout.
    // The application must explicitly call DrawCarouselScene instead of
    // DrawScene to use the experimental rendering path.
    enum class ESplitScreenSettings_Extra_Carousel
    {
        // Useless, but fun to try to create: 
        
        // Side-by-side camera views move right.
        // The part leaving the right edge reappears at the left edge.
        CarouselVerticalSplit,      

        // Stacked camera views move down.
        // The part leaving the bottom edge reappears at the top edge.
        CarouselHorizontalSplit,    

        // Cameras exchange grid positions in clockwise angular order.
        // This version jumps between positions rather than sliding.
        // A tile at the framebuffer center remains stationary.
        CarouselGridSplit           
    };
    
    struct CarouselSettings
    {
        ESplitScreenSettings_Extra_Carousel layout =
            ESplitScreenSettings_Extra_Carousel::CarouselVerticalSplit;

        // Duration of one complete cycle.
        //
        // Strip modes:
        //   Time to travel one whole framebuffer width or height.
        //
        // Grid mode:
        //   Time to visit all moving grid positions and return to the
        //   starting arrangement.
        //
        // UpdateCarousel pauses progression when this value is <= 0.
        float secondsPerLoop = 8.0f;

        
        // Progress through one cycle, stored independently of pixel size.
        //
        // 0.00 = beginning
        // 0.25 = one quarter completed
        // 0.50 = halfway completed
        //
        // UpdateCarousel wraps the value before it reaches another full
        // cycle, maintaining 0 <= phase < 1 for valid finite inputs.
        //
        // Strip modes turn this into a pixel displacement.
        // Grid mode turns it into a whole-number position shift.
        float phase = 0.0f;
    };

    
    
    // Advances only the carousel's timing.
    // Call once per simulation frame, not once per camera.
    //
    // deltaTime is elapsed time in seconds.
    // This does not move cameras in the world or update model transforms.
    //
    // Assumes finite timing values and an initially valid phase.
    void UpdateCarousel(CarouselSettings& settings, float deltaTime);

    
    
    // Draws the scene through the experimental layout.
    //
    // The original scene's cameras and layout settings are not modified.
    // Temporary camera copies provide the experimental viewport layout;
    // model instances and GPU resources remain shared with the real scene.
    //
    // Call instead of DrawScene for this frame, not in addition to it:
    // both rendering paths clear the framebuffer.
    //
    // Assumes the normal renderer's depth testing, render settings, and
    // color/depth write masks are already configured appropriately.
    //
    // Leaves scissor testing disabled. Other OpenGL state, including the
    // last viewport, is not restored to its value before this call.
    void DrawCarouselScene(
        const Scene& scene,
        const ShaderProgram& shader,
        const FramebufferSize& framebuffer,
        const CarouselSettings& settings);
    
    
    
}

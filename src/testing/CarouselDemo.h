#pragma once


struct Scene;
struct FramebufferSize;
class ShaderProgram;





namespace Demo
{
    
    enum class SplitScreenSettings_Extra_Carousel
    {
        // Useless, but fun to try to create: 
        CarouselVerticalSplit,      // Moving over screen sideways Left to Right
        CarouselHorizontalSplit,    // Moving over screen from Up to Down screen
        CarouselGridSplit           // Circling clockwise 
    };
    
    struct CarouselSettings
    {
        SplitScreenSettings_Extra_Carousel layout =
            SplitScreenSettings_Extra_Carousel::CarouselVerticalSplit;

        float secondsPerLoop = 8.0f;

        // Fraction of one complete journey across the framebuffer.
        // Always kept between 0 and 1.
        float phase = 0.0f;
    };

    void UpdateCarousel(CarouselSettings& settings, float deltaTime);

    void DrawCarouselScene(
        const Scene& scene,
        const ShaderProgram& shader,
        const FramebufferSize& framebuffer,
        const CarouselSettings& settings);
    
    
    
}

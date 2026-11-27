#pragma once

// Project headers
#include "Texture.h"

// Third-party libraries
#include <glm/vec3.hpp>
#include <flecs.h> 




// How should material color + texture be handled? Override, tint, some setting for choice of behavior? 
// Is white no color or white
// Tint blending, opacity and blend strength 

// Shader behavior? 
// Choose a color by blending vertex color toward baseColor.
// Without a texture, use that chosen color directly.
// With a texture, use that chosen color to calculate a brightness/color adjustment.
// Apply lighting.

// need a way to tell if vertexColor? Handle in shader



namespace AcornData
{
    
    struct MaterialId
    {
        // Flecs entity containing Material asset. 
        // Zero means no model asset is assigned.
        flecs::entity_t value = 0;
    };
    
    
    // Surface parameters interpreted by the current basic shader.
    // Materials reference shared textures; they do not own GPU images.
    struct Material
    {
        glm::vec3 baseColor{1.0f};
        TextureId diffuseTexture{};
        
        // 0 = material color, 1 = vertex color.
        // Renderer falls back to material color when vertex colors are absent.
        float vertexColorWeight = 0.0f;
        
        // Strength of the existing texture brightness/color adjustment.
        // Zero preserves sampled texture RGB.
        float textureTintStrength = 0.0f;
        
        // MIGRATION: preserve an explicitly requested but missing texture.
        // Do not remove until texture loading retains failure information
        // independently from "no texture requested".
        bool textureExpected = false;
    };
    
}



        
        
        //TextureId specularTexture{};
        //TextureId normalTexture{};
        //TextureId heightTexture{};
        

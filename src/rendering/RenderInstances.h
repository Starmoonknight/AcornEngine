#pragma once

// Project headers
#include "AcornMath.h"
#include "Material.h"
#include "assets/Model.h"

// Third-party libraries
#include <glad/glad.h>                          // GLuint,  Keep glad include before GLFW include 
#include <glm/glm.hpp>                          // Vectors, matrices, dot and cross products



namespace AcornData
{

    struct GpuMesh;
    struct GpuModel;
    
    
    struct ModelRenderSettings
    {
        bool visible = true; 
        bool lightingRequested = false; 
        
        // ---- Remove settings below after refactor: 
        // MIGRATION: inputs used by old scene descriptions and demo builders.
        // Assets::MakeMaterial copies them into a Material.
        // DrawModel does not read these appearance values directly.
        // Remove when scene serialization/builders use material assignments.
        
        // for tinting the whole mesh
        bool overrideColor = false;         // will be removed at material switch, the vertexColorWeight answers both this and tintAlpha 
        float tintAlpha = 1.0f;             // in material: vertexColorWeight, but opposite - with vertexColorWeight 1 being no override color   
        glm::vec3 tintColor{1.0f};    // in new material: baseColor
        
        bool textureExpected = false;       // in new material: TextureId having a value 
        float tintTextureAlpha = 0.0f;      // in new material: textureTintStrength
        
    };
    
    
    // One placed use of a shared ModelAsset.
    // Owns placement and local overrides, but not GPU geometry or textures.
    struct ModelInstance
    {
        ModelAssetId asset{};
        
        // World placement of this instance.
        // This value will move into scene ECS component storage later.
        Transform transform{};  
        
        MaterialId wholeMaterialOverride{};
        
        // Empty, or one optional entry per asset node.
        std::vector<std::optional<Transform>> nodeOverrides;

        ModelRenderSettings renderSettings{}; 
    };
    
}


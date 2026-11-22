#pragma once

// Project headers
#include "AcornMath.h"

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
        
        // for tinting the whole mesh
        bool overrideColor = false;
        float colorTintAmount = 1.0f; 
        glm::vec3 tintColor{1.0f};
        
        bool textureExpected = false;
        float tintTextureAmount = 0.0f;
        
        bool lightingRequested = false; 
    };
    
    
    // 	One scene instance with a reference to a GpuModel, a transform, and any rendering overrides
    struct RenderModelInstance
    {
        const GpuModel* gpuModel = nullptr;
        Transform transform{};
        
        // for tinting the whole mesh
        //bool overrideColor = false;
        //glm::vec3 solidColor{1.0f};
        
        GLuint textureID = 0; // 0 means no texture selected for this object.
        ModelRenderSettings renderSettings{}; 
    };
    
}


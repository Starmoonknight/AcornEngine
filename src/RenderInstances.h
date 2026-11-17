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

    
    // 	One scene instance with a reference to a single GpuMesh, a transform, and any rendering overrides
    struct MeshInstance
    {        
        const GpuMesh* mesh = nullptr;
        Transform transform{};
        
        bool overrideColor = false;
        glm::vec3 solidColor{1.0f};
        
        GLuint textureID = 0; // 0 means no texture selected for this object.
    };
    
    // 	One scene instance with a reference to a GpuModel, a transform, and any rendering overrides
    struct ModelInstance
    {
        const GpuModel* gpuModel = nullptr;
        Transform transform{};
        
        // for tin´tint the whole mesh
        bool overrideColor = false;
        glm::vec3 solidColor{1.0f};
        
        GLuint textureID = 0; // 0 means no texture selected for this object.
    };
    
}


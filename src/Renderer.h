#pragma once

// Project headers
#include "ShaderProgram.h"

// Standard library
#include <vector>                               // Mesh data and temporary OBJ attribute lists

// Third-party libraries
#include <glad/glad.h>                          // GLuint,  Keep glad include before GLFW include 
#include <glm/glm.hpp>                          // Vectors, matrices, dot and cross products


namespace AcornData
{
    struct GpuMesh;
    struct ModelInstance;
}


namespace Rendering
{    
    
    
    void SetBasicShaderUniforms_Placement(
        const BasicUniformLocations& uniforms,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection);
    
    void SetBasicShaderUniforms_Visuals(
        const BasicUniformLocations& uniforms,
        bool colorOverride = false,
        glm::vec3 color = glm::vec3(1.0f),
        float colorAmount = 1.0,
        bool useTexture = false,
        float textureColorTintAmount = 0.0);
    
    void SetBasicShaderUniforms(
        const ShaderProgram& shader,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection,
        bool colorOverride = false,
        glm::vec3 color = glm::vec3(1.0f),
        float colorAmount = 1.0f,
        bool useTexture = false,
        float textureColorTintAmount = 0.0f); 
    
    
    
    void DrawMesh(const AcornData::GpuMesh& mesh); 
    
    
    void DrawMesh_FaceColors(
        const ShaderProgram& shader, 
        const AcornData::GpuMesh& mesh,  
        const std::vector<glm::vec3>& faceColors);
    
    
    
    void DrawModel(
        const ShaderProgram& shaderProgram, const glm::mat4& view, const glm::mat4& projection, 
        const AcornData::ModelInstance& model, const glm::mat4 modelMatrix, GLuint modelTexture);
    
}



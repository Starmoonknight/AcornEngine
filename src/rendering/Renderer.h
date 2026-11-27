#pragma once

// Project headers

// Standard library
#include <vector>                               // Mesh data and temporary OBJ attribute lists

// Third-party libraries
#include <glad/glad.h>                          // GLuint,  Keep glad include before GLFW include 
#include <glm/glm.hpp>                          // Vectors, matrices, dot and cross products
#include <flecs.h>

class ShaderProgram;
struct BasicUniformLocations;



namespace AcornData
{
    
    struct GpuMesh;
    struct ModelInstance;   
    
}




namespace Rendering
{
    
    struct CameraMatrices;
    
    
    struct RenderSettings
    {
        bool wireframe = false;
        bool cullFaces = true;
    };
    
    
    void ApplyRenderSettings(const RenderSettings& settings);
    
    
    
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
    

    // Draw one model instance through one camera.
    // Resolves shared assets/materials/textures, evaluates node world matrices,
    // and draws every node's parts using their effective material.
    //    
    // Does not advance animation or modify scene state.
    // Note:  Currently recalculates matrices for each camera draw.
    // Requires valid model structure and a current OpenGL context.
    void DrawModel(
        flecs::world& world,
        const ShaderProgram& shader, 
        const CameraMatrices& viewMatrices, 
        const AcornData::ModelInstance& model);
    
    
    
    
    
    
    // This function is becoming obsolete with the new draw path, needs updating or will be removed 
    void DrawMesh_FaceColors(
        const ShaderProgram& shader, 
        const AcornData::GpuMesh& mesh,  
        const std::vector<glm::vec3>& faceColors);
    
    
    
}



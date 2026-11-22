#pragma once

// Project headers

// Standard library
#include <vector>                               // Mesh data and temporary OBJ attribute lists

// Third-party libraries
#include <glad/glad.h>                          // GLuint,  Keep glad include before GLFW include 
#include <glm/glm.hpp>                          // Vectors, matrices, dot and cross products


class ShaderProgram;
struct BasicUniformLocations;



namespace AcornData
{
    
    struct GpuMesh;
    struct RenderModelInstance;
    
    
    
    
    // unused for now
    enum class TextureType
    {
        Diffuse,
        Specular,
        Normal,
    };
    // unused for now
    struct Texture {
        GLuint id = 0;
        TextureType type;   // type e.g. a diffuse or specular texture. 
    };      
    
}




namespace Texture
{
    
    GLuint LoadTexture2D(const char* filePath);
    
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
    
    
    // Note: 
    // Currently will have to re-calculate the model matrix for each camera instead of once and then applied to each camera draw
    void DrawModel(const ShaderProgram& shader, const CameraMatrices& viewMatrices, const AcornData::RenderModelInstance& model);
    
    
    
    
    
    
    // This function is becoming obsolete with the new draw path, needs updating or will be removed 
    void DrawMesh_FaceColors(
        const ShaderProgram& shader, 
        const AcornData::GpuMesh& mesh,  
        const std::vector<glm::vec3>& faceColors);
    
    
    
}



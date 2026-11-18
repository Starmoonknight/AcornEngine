#include "Renderer.h"


// Project headers 
#include "GpuModel.h"
#include "OpenGLErrors.h"
#include "RenderInstances.h"

// Third-party libraries
#include <cstddef>                          // std::size_t and offsetof for vertex attributes
#include <cassert>                          // assert checks
#include <glm/gtc/type_ptr.hpp>             // glm::value_ptr for OpenGL uniforms



namespace Rendering
{    
    
    
    void SetBasicShaderUniforms_Placement(
        const BasicUniformLocations& uniforms,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection)
    {
        // updates uniforms on the currently active program using the saved uniform locations 
        glUniformMatrix4fv(uniforms.modelLocation, 1 ,GL_FALSE,  glm::value_ptr(model));
        glUniformMatrix4fv(uniforms.viewLocation, 1 ,GL_FALSE,  glm::value_ptr(view));
        glUniformMatrix4fv(uniforms.projectionLocation, 1 ,GL_FALSE,  glm::value_ptr(projection));
    }
    
    void SetBasicShaderUniforms_Visuals(
        const BasicUniformLocations& uniforms,
        bool colorOverride,
        glm::vec3 color,
        float colorAmount,
        bool useTexture,
        float textureColorTintAmount)
    {
        glUniform1i(uniforms.overrideColorLocation, colorOverride);
        glUniform3fv(uniforms.colorLocation, 1, glm::value_ptr(color)); 
        glUniform1f(uniforms.colorAmountLocation, colorAmount);
        
        glUniform1i(uniforms.useTextureLocation, useTexture);
        glUniform1f(uniforms.tintTextureColorAmountLocation, textureColorTintAmount);
    }
    
    void SetBasicShaderUniforms(
        const ShaderProgram& shader,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection,
        bool colorOverride,
        glm::vec3 color,
        float colorAmount,
        bool useTexture,
        float textureColorTintAmount)
    {
        shader.use();
        const BasicUniformLocations& uniforms = shader.GetBasicUniformLocations();
        
        SetBasicShaderUniforms_Placement(uniforms, model, view, projection);
        SetBasicShaderUniforms_Visuals(uniforms, colorOverride, color, colorAmount, useTexture, textureColorTintAmount); 
        CheckOpenGLErrors("setting geometry Uniform's");
    }
    
    
    
    
    void DrawMesh(const AcornData::GpuMesh& mesh)
    {
        assert(mesh.vao != 0);
        
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
        
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing mesh");
    }
    
    void DrawMesh_FaceColors(
        const ShaderProgram& shader, 
        const AcornData::GpuMesh& mesh,  
        const std::vector<glm::vec3>& faceColors)
    {
        assert(mesh.vao != 0);
        assert(mesh.index_count >= 0);
        
        const auto& faces = mesh.faces;
        
        assert(!faces.empty());
        assert(faces.size() == faceColors.size());
        
        const std::size_t totalIndexCount = static_cast<std::size_t>(mesh.index_count);
        
        // Check that each range fits within the uploaded index list.
        for (const AcornData::FaceRange& range : faces)
        {
            assert(range.indexCount > 0);
            assert(range.indexCount % 3 == 0);
            assert(range.firstIndex <= totalIndexCount);
            assert(range.indexCount <= totalIndexCount - range.firstIndex);
        }
        
        
        shader.use(); 
        glBindVertexArray(mesh.vao);
        
        const auto& uniforms = shader.GetBasicUniformLocations();
        
        // This function requires a shader with these active uniforms.
        assert(uniforms.overrideColorLocation != -1);
        assert(uniforms.colorLocation != -1);
        
        glUniform1i(uniforms.overrideColorLocation, GL_TRUE);
        
        for (std::size_t face = 0; face < faces.size(); ++face)
        {
            const AcornData::FaceRange& range = faces[face];
            const std::size_t offsetBytes = range.firstIndex * sizeof(unsigned int);
            
            glUniform3fv(uniforms.colorLocation, 1, glm::value_ptr(faceColors[face]));
            
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.indexCount), GL_UNSIGNED_INT, reinterpret_cast<const void*>(offsetBytes));
        }
        
        glUniform1i(uniforms.overrideColorLocation, GL_FALSE);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing mesh with different face colors");
    }
    
    
    
    void DrawModel(
        const ShaderProgram& shaderProgram, const glm::mat4& view, const glm::mat4& projection, 
        const AcornData::ModelInstance& model, const glm::mat4& modelMatrix, 
        GLuint modelTexture, bool lightingRequested, float colorAmount, float textureColorTintAmount)
    {
        if (model.gpuModel == nullptr)
        {
            return;
        }
        
        shaderProgram.use();
        glUniform1i(shaderProgram.GetBasicUniformLocations().textureLocation, 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, modelTexture); 
        
        for (const auto& mesh : model.gpuModel->meshes)
        {
            const bool useTexture = mesh.hasUVs && modelTexture != 0;
            const bool useLighting = lightingRequested && mesh.hasNormals;
            
            SetBasicShaderUniforms(
                shaderProgram, modelMatrix, view, projection,
                model.overrideColor, model.solidColor, colorAmount, useTexture, textureColorTintAmount
            );
            
            shaderProgram.SetBool("uUseLighting", useLighting);
            DrawMesh(mesh);
        }
    }
    
}


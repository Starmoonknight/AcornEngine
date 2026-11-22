#include "Renderer.h"


// Project headers 
#include "Camera.h"
#include "GpuModel.h"
#include "OpenGLErrors.h"
#include "RenderInstances.h"
#include "ShaderProgram.h"

// Standard library
#include <cassert>                          // assert checks
#include <cstddef>                          // std::size_t and offsetof for vertex attributes
#include <iostream>

// Third-party libraries
#include <glm/gtc/type_ptr.hpp>             // glm::value_ptr for OpenGL uniforms
#include "stb_image.h"




namespace Texture
{
    
    GLuint LoadTexture2D(const char* filePath)
    {
        int width = 0;
        int height = 0;
        int channelsInFile = 0;
        
        // Return four bytes per pixel (RGBA), regardless of the file's channels.  Should work with both .jpg and .png 
        unsigned char *pixels = stbi_load(filePath, &width, &height, &channelsInFile, STBI_rgb_alpha);  
        
        if (!pixels)
        {
            std::cerr << "Failed to load texture: " << filePath << '\n';
            return 0;
        }
        
        GLuint textureID = 0;
        glGenTextures(1, &textureID);  
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // load and generate the texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // releases the CPU-side pixel array returned by stbi_load() after glTexImage2D() has consumed the pixels for the OpenGL texture
        stbi_image_free(pixels);

        return textureID;
    }
    
    
}



namespace Rendering
{
    
    
    void ApplyRenderSettings(const RenderSettings& settings)
    {
        glPolygonMode(
            GL_FRONT_AND_BACK,
            settings.wireframe ? GL_LINE : GL_FILL);

        if (settings.cullFaces)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
    }
    
    
    
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
    
    
    void DrawModel(const ShaderProgram& shader, const CameraMatrices& viewMatrices, const AcornData::RenderModelInstance& model)
    {
        
        if (!model.renderSettings.visible || model.gpuModel == nullptr)
        {
            return;
        }

        const glm::mat4 modelMatrix = AcornMath::BuildModelMatrix(model.transform);
        
        const auto& settings = model.renderSettings;
        
        
        shader.use();
        glUniform1i(shader.GetBasicUniformLocations().textureLocation, 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model.textureID); 
        
        for (const auto& mesh : model.gpuModel->meshes)
        {
            const bool missingTexture =
                 settings.textureExpected && model.textureID == 0;

            const bool missingUVs =
                settings.textureExpected && !mesh.hasUVs;

            const bool showTextureError =
                missingTexture || missingUVs;
            
            
            const bool useTexture =
                !showTextureError &&
                mesh.hasUVs &&
                model.textureID != 0;

            // Also disable lighting if texture error so the diagnostic remains bright.
            const bool useLighting =
                !showTextureError &&
                settings.lightingRequested &&
                mesh.hasNormals;

            
            // If textures are missing but were expected show error color
            const bool overrideColor =
                showTextureError || settings.overrideColor;

            const glm::vec3 color =
                showTextureError
                    ? glm::vec3(1.0f, 0.0f, 1.0f)
                    : settings.tintColor;

            const float colorAmount =
                showTextureError ? 1.0f : settings.colorTintAmount;

            const float textureTintAmount =
                showTextureError ? 0.0f : settings.tintTextureAmount;
            
            
            
            SetBasicShaderUniforms(
                shader, modelMatrix, viewMatrices.view, viewMatrices.projection,
                overrideColor, color, colorAmount, useTexture, textureTintAmount
            );
            
            shader.SetBool("uUseLighting", useLighting);
            DrawMesh(mesh);
        }
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
    
    
}


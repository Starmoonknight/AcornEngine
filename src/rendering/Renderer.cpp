#include "Renderer.h"


// Project headers 
#include "assets/AssetRecords.h"
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
    
    
    void DrawModel(
        flecs::world& world,
        const ShaderProgram& shader, 
        const CameraMatrices& viewMatrices, 
        const AcornData::ModelInstance& model)
    {
        using namespace AcornData;
        
        if (!model.renderSettings.visible)
        {
            return;
        }
        
        const auto* asset = 
            Assets::Find<ModelAsset>(world, model.asset.value);

        if (asset == nullptr || asset->geometry == nullptr)
            return;
        
        
        std::vector<glm::mat4> nodeMatrices;
        asset->BuildNodeMatrices(
            model.transform,
            model.nodeOverrides,
            nodeMatrices);
        
        
        // Fallback drawing defaults when the selected material cannot be resolved.
        // This local value is not registered in Flecs and has no asset identity.
        // Its defaults produce an untextured white surface.
        const Material fallback{};
        
        shader.use();
        // Tell this shader's sampler to read texture unit index 0.
        glUniform1i(shader.GetBasicUniformLocations().textureLocation, 0);
        // Select which unit subsequent glBindTexture calls modify.
        // Selecting this unit does not disable other units.
        glActiveTexture(GL_TEXTURE0);
        
        for (std::size_t nodeIndex = 0; nodeIndex < asset->nodes.size(); ++nodeIndex)
        {
            const ModelNode& node = asset->nodes[nodeIndex]; 
            
            for (const ModelPart& part : node.parts)
            {
                const GpuMesh& mesh = 
                    asset->geometry->meshes[part.meshIndex]; 
                
                // Instance whole-material override
                MaterialId selected = model.wholeMaterialOverride;
                // Zero in the checks below means "no replacement", so try the next assignment level.
                // A nonzero but invalid reference falls back to the emergency material;
                // it does not silently reveal a lower-priority assignment.
                
                //      otherwise asset whole-material override
                if (selected.value == 0)
                {
                    selected = asset->wholeMaterialOverride;
                }
                
                //      otherwise part’s material-slot assignment
                if (selected.value == 0)
                {
                    selected =
                       asset->materialSlots[part.materialSlotIndex].material;
                }
                
                const Material* found =
                Assets::Find<Material>(world, selected.value);
                //      otherwise fallback material
                const Material& material = found ? *found : fallback;

                const AcornData::Texture* texture = 
                    Assets::Find<AcornData::Texture>(world, material.diffuseTexture.value);

                // Bind this part's texture object to the GL_TEXTURE_2D binding on unit 0.
                // The shader's sampler reads this binding when DrawMesh executes.
                //  Note:   Current material can only have one texture so always using 
                //          this slot until more texture options are supported. 
                const GLuint textureObject = texture ? texture->objectId : 0;
                
                const bool expectsTexture =
                    material.textureExpected ||
                    material.diffuseTexture.value != 0;

                const bool textureError =
                    expectsTexture &&
                    (textureObject == 0 || !mesh.hasUVs);

                const bool useTexture =
                    !textureError &&
                    textureObject != 0 &&
                    mesh.hasUVs;
                
                
                // Contribution of vertex color:
                // 0 = material color only; 1 = vertex color only.
                // Without a color buffer, force material color instead of depending
                // on the value of a disabled vertex attribute.
                const float vertexWeight =
                    mesh.colorVBO != 0
                    ? material.vertexColorWeight
                    : 0.0f;
                
                const glm::vec3 color =
                    textureError
                    ? glm::vec3(1.0f, 0.0f, 1.0f)
                    : material.baseColor;

                // The existing shader uses mix(VertexColor, ObjectColor, amount).
                // Its amount therefore measures MATERIAL contribution, the opposite
                // of vertexWeight. A texture error forces the full diagnostic color.
                const float materialWeight = 
                    textureError ? 1.0f : 1.0f - vertexWeight;
                
                
                // Also disable lighting if texture error so the diagnostic remains bright.
                const bool useLighting = 
                    !textureError &&
                    model.renderSettings.lightingRequested &&
                    mesh.hasNormals;
                    

                glBindTexture(GL_TEXTURE_2D, textureObject);
                
                SetBasicShaderUniforms(
                    shader,
                    nodeMatrices[nodeIndex],
                    viewMatrices.view,
                    viewMatrices.projection,
                    true,
                    color,
                    materialWeight,
                    useTexture,
                    textureError ? 0.0f : material.textureTintStrength);

                shader.SetBool("uUseLighting", useLighting);
                
                DrawMesh(mesh);
            }
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


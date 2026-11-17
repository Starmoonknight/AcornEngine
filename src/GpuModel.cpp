#include "GpuModel.h"


// Project headers 
#include "OpenGLErrors.h"

// Standard library
#include <cassert>                  // assert checks
#include <cstddef>                  // std::size_t and offsetof for vertex attributes
#include <limits>                   // std::numeric_limits for bounds checks


// Third-party libraries
#include <glm/glm.hpp>              // Vectors, matrices, dot and cross products



namespace Rendering
{
    
    AcornData::GpuMesh CreateGpuMesh(const AcornData::CpuMeshData& mesh, GLenum usage)
    {
        using AcornData::Vertex;
        using AcornData::GpuMesh;
        // Colors are absent, or there is one color per vertex.
        assert(mesh.colors.empty() || mesh.colors.size() == mesh.vertices.size());
        
        // Triangle indices come in groups of three.
        assert(mesh.indices.size() % 3 == 0);       
        // Every index refers to an existing vertex.
        for (unsigned int index : mesh.indices)     
        {
            assert(index < mesh.vertices.size());
        }
        // The total index count fits the draw-call parameter.
        assert(mesh.indices.size() <= static_cast<std::size_t>((std::numeric_limits<GLsizei>::max)()));  
        
        
        // ----- Setup -----
        
        GpuMesh gpu; 
        gpu.index_count = static_cast<GLsizei>(mesh.indices.size());
        gpu.faces = mesh.faces;
        
        glGenVertexArrays(1, &gpu.vao);
        glGenBuffers(1, &gpu.vertexVBO);
        glGenBuffers(1, &gpu.ebo);
        glBindVertexArray(gpu.vao);
        
        
        // ----- Vertex VBO -----
        glBindBuffer(GL_ARRAY_BUFFER, gpu.vertexVBO);
        
        GLsizeiptr verticesSizeBytes = static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex));          // number of elements x bytes per element.
        glBufferData(GL_ARRAY_BUFFER, verticesSizeBytes, mesh.vertices.data(), usage);
        
        // position,        in attribute slot 0
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 
            static_cast<GLsizei>(sizeof(Vertex)), 
            reinterpret_cast<const void*>(offsetof(Vertex, position))
        );
        glEnableVertexAttribArray(0);
        
        // normal,          in attribute slot 2
        gpu.hasNormals = mesh.hasNormals;
        if (mesh.hasNormals)
        {
            glVertexAttribPointer(
                1, 3, GL_FLOAT, GL_FALSE, 
                static_cast<GLsizei>(sizeof(Vertex)), 
                reinterpret_cast<const void*>(offsetof(Vertex, normal))
            );
            
            glEnableVertexAttribArray(1);
        }
        else
        {
            glDisableVertexAttribArray(1);
        }
        
        // uv,              in attribute slot 3
        gpu.hasUVs = mesh.hasUVs;
        if (mesh.hasUVs)
        {
            glVertexAttribPointer(
                2, 2, GL_FLOAT, GL_FALSE,
                static_cast<GLsizei>(sizeof(Vertex)),
                reinterpret_cast<const void*>(offsetof(Vertex, uv))
            );
            
            glEnableVertexAttribArray(2);
        }
        else
        {
            glDisableVertexAttribArray(2);
        }
        CheckOpenGLErrors("geometry buffer and attribute setup");
        
        
        // ----- Color VBO -----
        if (!mesh.colors.empty())
        {
            glGenBuffers(1, &gpu.colorVBO);
            glBindBuffer(GL_ARRAY_BUFFER, gpu.colorVBO);
            
            // color buffer
            GLsizeiptr colorsSizeBytes = static_cast<GLsizeiptr>(mesh.colors.size() * sizeof(glm::vec3)); 
            glBufferData(GL_ARRAY_BUFFER, colorsSizeBytes, mesh.colors.data(), usage);
            
            // color attribute
            glVertexAttribPointer(
                3, 3, GL_FLOAT, GL_FALSE, 
                static_cast<GLsizei>(sizeof(glm::vec3)), 
                nullptr
            );
            glEnableVertexAttribArray(3);
        }
        else
        {
            glDisableVertexAttribArray(3);
        }
        CheckOpenGLErrors("color buffer and attribute setup");
        
        
        // ----- EBO -----
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
        
        GLsizeiptr indicesSizeBytes = static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(unsigned int)); 
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSizeBytes, mesh.indices.data(), usage);
        CheckOpenGLErrors("EBO buffer setup");
        
        
        // ----- Finish setup -----
        glBindVertexArray(0);
        CheckOpenGLErrors("mesh setup");

        return gpu; 
    }
    
    void DestroyGpuMesh(AcornData::GpuMesh& mesh)
    {
        glDeleteVertexArrays(1, &mesh.vao);

        glDeleteBuffers(1, &mesh.vertexVBO);
        glDeleteBuffers(1, &mesh.colorVBO);
        glDeleteBuffers(1, &mesh.ebo);

        mesh = AcornData::GpuMesh{};
    }
    
    
    AcornData::GpuModel CreateGpuModel(const std::vector<AcornData::CpuMeshData>& modelMeshes, GLenum usage)
    {
        AcornData::GpuModel gpuModel;
        gpuModel.meshes.reserve(modelMeshes.size());

        for (const auto& mesh : modelMeshes)
        {
            gpuModel.meshes.push_back(CreateGpuMesh(mesh, usage));
        }

        return gpuModel;
    }
    
    void DestroyGpuModel(AcornData::GpuModel& model)
    {
        for (auto& mesh : model.meshes)
        {
            DestroyGpuMesh(mesh);
        }

        model.meshes.clear();
    }
    
    
}
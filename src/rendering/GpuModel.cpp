#include "GpuModel.h"


// Project headers 
#include "OpenGLErrors.h"

// Standard library
#include <cassert>                  // assert checks
#include <cstddef>                  // std::size_t and offsetof for vertex attributes
#include <limits>                   // std::numeric_limits for bounds checks
#include <utility>                      // For std::exchange and std::move


// Third-party libraries
#include <glm/glm.hpp>              // Vectors, matrices, dot and cross products


namespace AcornData
{
    
    void GpuMesh::Reset() noexcept
    {
        // Calling Reset() does not destroy the C++ object. The variable still exists and can later receive another upload through move assignment.
        
        // A zero handle means this object owns no resource in that field.
        // For a nonzero handle, ask OpenGL to delete the named resource.
        //
        // 1 means "one resource".
        // &vao supplies the address of the variable containing its handle.
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vertexVBO != 0) glDeleteBuffers(1, &vertexVBO);
        if (colorVBO != 0) glDeleteBuffers(1, &colorVBO);
        if (ebo != 0) glDeleteBuffers(1, &ebo);

        // The OpenGL deletion functions do not reset the C++ variables, 
        // forget the old handles so later cleanup does not delete them again.
        vao = 0;
        vertexVBO = 0;
        colorVBO = 0;
        ebo = 0;
        
        // This object now represents no drawable geometry.
        index_count = 0;
        
        // Remove CPU-side face-range records.
        // clear() removes elements; it does not release vector capacity.
        faces.clear();
        // An empty mesh has no available normal or UV attributes.
        hasNormals = false;
        hasUVs = false;
    }

    
    void GpuMesh::TakeFrom(GpuMesh& other) noexcept
    {
        // Take each handle and replace the source's handle with zero.
        // This transfers cleanup responsibility; it does not copy GPU data.
        vao = std::exchange(other.vao, 0);
        vertexVBO = std::exchange(other.vertexVBO, 0);
        colorVBO = std::exchange(other.colorVBO, 0);
        ebo = std::exchange(other.ebo, 0);

        // Transfer the draw count and leave the source with nothing to draw.
        index_count = std::exchange(other.index_count, 0);
        
        // Move the CPU-side face-range vector into this object.
        faces = std::move(other.faces);
        // Explicitly leave the source's face list empty.
        other.faces.clear();

        // Transfer attribute availability and clear the source's flags.
        hasNormals = std::exchange(other.hasNormals, false);
        hasUVs = std::exchange(other.hasUVs, false);
    }
    
    
    // Exactly one GpuMesh is responsible for deleting a buffer. 
    // Moving transfers that responsibility, copying is forbidden.
    GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    {
        // Before this body runs, the field initializers should have already run:
        // handles are 0, faces is empty, and flags are false.
        //
        // This new object therefore has no old GPU resources to release.
        TakeFrom(other);
    }

    GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept
    {
        // 'this' points to the receiving object.
        // '&other' is the address of the source object.
        //
        // Do nothing if someone tries to move an object into itself.
        if (this != &other)
        {
            // Release the resources this object already owns.
            // Otherwise, their handles would be overwritten and lost.
            Reset();
            
            // Receive the source's resources and leave the source empty.
            TakeFrom(other);
        }
        
        // Return the receiving object by reference.
        return *this;
    }

    GpuMesh::~GpuMesh()
    {
        // Automatically release whatever this object still owns.
        // An empty or moved-from object has no nonzero handles to delete.
        Reset();
    }
    
}



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
        // Explicit early cleanup uses the same implementation as destruction.
        mesh.Reset();  
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

    AcornData::GpuModel CreateGpuModel(const AcornData::CpuMeshData& mesh, GLenum usage)
    {
        AcornData::GpuModel result;
        result.meshes.push_back(CreateGpuMesh(mesh, usage));
        return result;
    }

    void DestroyGpuModel(AcornData::GpuModel& model)
    {
        // Destroy all mesh elements, releasing their GPU resources.
        // The model object remains alive with an empty vector.
        model.meshes.clear();
    }
    
    
}
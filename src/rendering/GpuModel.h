#pragma once

// Project headers
#include "geometry/CpuMeshData.h"

// Standard library
#include <vector>                       // Mesh data and temporary OBJ attribute lists

// Third-party libraries
#include <glad/glad.h>                  // GLuint,  Keep glad include before GLFW include 



namespace AcornData
{
    
    // GPU handles and drawing metadata 	The VAO, buffers, and draw information for one uploaded mesh
    //      one successful load and upload per path and settings combination. ? 
    struct GpuMesh
    {
        GLuint vao = 0;
        GLuint vertexVBO = 0;
        GLuint colorVBO = 0;
        GLuint ebo = 0;
        
        GLsizei index_count = 0;
        std::vector<FaceRange> faces;
        
        bool hasNormals = false;
        bool hasUVs = false;
        
        
        // ----- Usage rules enforcement ----- 
        
        // Create an empty mesh.
        // The field initializers above set handles to 0 and flags to false.
        GpuMesh() = default;
        // Called automatically when this object's lifetime ends.
        // Releases any OpenGL resources it still owns.
        ~GpuMesh();

        // Forbid creating a second owner by copying an existing mesh.
        // Example rejected by the compiler: GpuMesh second = first;
        GpuMesh(const GpuMesh&) = delete;           
        
        // Forbid copying an existing mesh into another existing mesh.
        // Example rejected by the compiler: second = first;
        GpuMesh& operator=(const GpuMesh&) = delete;        

        
        // Construct a new mesh by taking ownership from another mesh.
        // The source is left empty.
        GpuMesh(GpuMesh&& other) noexcept;                  // noexcept promises these operations will not throw C++ exceptions.
        
        // Replace this mesh's resources with resources taken from another mesh.
        // Releases this mesh's previous resources first; leaves the source empty.
        GpuMesh& operator=(GpuMesh&& other) noexcept;

        // Release owned resources and leave this object empty.
        // A current OpenGL context is required if any handles are nonzero.
        void Reset() noexcept;                              // After a Reset the object owns nothing, makes repeated reset safe and
                                                            // ensures later destruction does not release the same handles again. 

        private:
        // Transfer handles and metadata from other into this mesh.
        // This mesh must already be empty. Leaves other empty.
        void TakeFrom(GpuMesh& other) noexcept;             
    };
    
    // The collection of GpuMesh resources for one imported model
    struct GpuModel
    {
        std::vector<GpuMesh> meshes;

        
        // Trying out implementing special ownership behavior at the lowest resource-owning layer, then let containing types compose it.
        
        GpuModel() = default;
        // Member destruction automatically destroys the mesh vector.
        ~GpuModel() = default;
        
        GpuModel(const GpuModel&) = delete;
        GpuModel& operator=(const GpuModel&) = delete;
        
        // Let the vector handle transfer of its owned storage.
        GpuModel(GpuModel&&) noexcept = default;
        GpuModel& operator=(GpuModel&&) noexcept = default;
    };
    
    
}



namespace Rendering
{
    
    AcornData::GpuMesh CreateGpuMesh(const AcornData::CpuMeshData& mesh, GLenum usage = GL_STATIC_DRAW);
    
    void DestroyGpuMesh(AcornData::GpuMesh& mesh);
    
    
    AcornData::GpuModel CreateGpuModel(const std::vector<AcornData::CpuMeshData>& modelMeshes, GLenum usage = GL_STATIC_DRAW);
    
    AcornData::GpuModel CreateGpuModel(  const AcornData::CpuMeshData& mesh, GLenum usage = GL_STATIC_DRAW);
    
    void DestroyGpuModel(AcornData::GpuModel& model);
    
    
}


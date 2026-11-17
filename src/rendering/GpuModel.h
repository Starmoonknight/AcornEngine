#pragma once

// Project headers
#include "CpuMeshData.h"

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
    };
    
    // The collection of GpuMesh resources for one imported model
    struct GpuModel
    {
        std::vector<GpuMesh> meshes;
    };
    
    
}



namespace Rendering
{
    
    AcornData::GpuMesh CreateGpuMesh(const AcornData::CpuMeshData& mesh, GLenum usage = GL_STATIC_DRAW);
    
    void DestroyGpuMesh(AcornData::GpuMesh& mesh);
    
    
    AcornData::GpuModel CreateGpuModel(const std::vector<AcornData::CpuMeshData>& modelMeshes, GLenum usage = GL_STATIC_DRAW);
    
    void DestroyGpuModel(AcornData::GpuModel& model);
    
    
}


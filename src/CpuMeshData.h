#pragma once

// Standard library
#include <cstddef>              // std::size_t and offsetof for vertex attributes
#include <type_traits>          // std::is_standard_layout for Vertex
#include <vector>               // Mesh data and temporary OBJ attribute lists

// Third-party libraries
#include <glm/glm.hpp>          // Vectors, matrices, dot and cross products


namespace AcornData
{
    
    // describes one rendering vertex position plus associated attributes
    struct Vertex
    {        
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f};
        glm::vec2 uv{0.0f};
    };
    static_assert(
        std::is_standard_layout<Vertex>::value,
        "Vertex must support standard-layout member offsets"
    );
    
    // a set of consecutive indices that makes a full face 
    struct FaceRange
    {
        std::size_t firstIndex = 0;
        std::size_t indexCount = 0;
    };
    
    // CPU-side vertices, indices, and other data produced by the loader
    struct CpuMeshData
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices; 
        
        //std::vector<Texture> textures;    // in MeshData or in GpuMesh? 
        std::vector<glm::vec3> colors;          // Empty when this mesh has no per-vertex hard coded colors.
        
        std::vector<FaceRange> faces;        
        
        bool hasNormals = false;
        bool hasUVs = false;
    };
    
    
    
}
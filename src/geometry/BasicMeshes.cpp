#include "BasicMeshes.h"

// Project headers
#include "Palette.h"

// Standard library
#include <cmath>                // Scalar math for geometry and animation
#include <cstddef>              // std::size_t and offsetof for vertex attributes
#include <iostream>             // std::cout and std::cerr for diagnostics
#include <vector>               // std::vector mesh data 

// Third-party libraries
#include <glm/glm.hpp>          // Vectors, matrices, dot and cross products



namespace 
{
        // It's not the nicest sphere, nor the nicest code, but it sort of works.
    void generateSortofSphere(std::vector<float> &vertices, std::vector<unsigned int> &indices, int rings = 9, int slices = 16) {

        if (rings < 3)  { std::cout << "no sphere generated. rings value < 3"  << std::endl;  return; }
        if (slices < 4) { std::cout << "no sphere generated. slices value < 4" << std::endl;  return; }
        
        float PIE = 3.1415f;
        
        vertices.clear();
        indices.clear();

        float angle_step    = 360.0f / slices; 
        float current_angle = 0.0f;
        
        // Vertices
        float y_step = 2.0f / (rings - 1);
        float y_pos  = 1.0f;

        for (int ring = 0; ring < rings; ring++) {

            int iteration_slices = slices;
            if (ring == 0 || ring == rings - 1) { iteration_slices = 1; }

            for (int i = 0; i < iteration_slices; i++) {

                float radius = sinf(acos(y_pos));

                float x_pos = cosf(current_angle * PIE / 180) * radius;
                float z_pos = sinf(current_angle * PIE / 180) * radius;

                vertices.push_back(x_pos);
                vertices.push_back(y_pos);
                vertices.push_back(z_pos);

                current_angle += angle_step;
            }

            y_pos -= y_step;
            current_angle = 0.0f;
        }

        std::cout << "Sphere vertex count:" << vertices.size() / 3 << std::endl;

        //Indices
        unsigned int top_most_index = 0;
        unsigned int bottom_most_index = vertices.size() / 3 - 1;

        for (int i = 0; i < rings - 1; i++) {

            for (int j = 0; j < slices; j++) {

                unsigned int tl_index = (i - 1) * slices + j + 1;
                unsigned int tr_index = (i - 1) * slices + j + 1 + 1;
                unsigned int bl_index = (i + 0) * slices + j + 1;
                unsigned int br_index = (i + 0) * slices + j + 1 + 1;

                if (j == slices - 1) { // For the seam
                    tr_index = (i - 2) * slices + j + 1 + 1;
                    br_index = (i - 1) * slices + j + 1 + 1;
                }

                if (i == 0) { // First ring

                    indices.push_back(top_most_index);
                    indices.push_back(br_index);
                    indices.push_back(bl_index);

                } else if (i == rings - 2) { // Last ring

                    indices.push_back(tr_index);
                    indices.push_back(bottom_most_index);
                    indices.push_back(tl_index);

                } else {

                    indices.push_back(tl_index);
                    indices.push_back(br_index);
                    indices.push_back(bl_index);

                    indices.push_back(tl_index);
                    indices.push_back(tr_index);
                    indices.push_back(br_index);
                }
            }
        }

        std::cout << "Sphere triangle count:" << indices.size() / 3 << std::endl;
    }
    
    
    AcornData::CpuMeshData generateSortofSphere_Color(int rings = 9, int slices = 16)
    {
        using AcornData::CpuMeshData;  
        
        CpuMeshData sphereMesh;  
        std::vector<float> spherePositions; 
        
        generateSortofSphere(spherePositions, sphereMesh.indices, rings, slices); 
        
        for (std::size_t i = 0; i < spherePositions.size(); i += 3)
        {
            // Position,    and reducing the generated diameter from 2 to 1.
            AcornData::Vertex vertex;
            vertex.position = glm::vec3(
                spherePositions[i], 
                spherePositions[i + 1],
                spherePositions[i + 2]
            ) * 0.5f;
            
            sphereMesh.vertices.push_back(vertex); 
            
            // Color
            sphereMesh.colors.push_back(Palette::orange);
        }
        
        return sphereMesh; 
    };    
    
}




namespace  BasicMeshes
{
    AcornData::CpuMeshData MakePyramid_Basic()
    {
        using AcornData::CpuMeshData;   
        
        CpuMeshData mesh;
        
        mesh.vertices = {
            { glm::vec3( 0.5f, -0.5f, -0.5f) }, // 0, bottom, back right 
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 1, bottom, back left 
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 2, bottom, front left 
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 3, bottom, front right
            { glm::vec3( 0.0f,  0.5f,  0.0f) }  // 4, top, middle
        };
        
        mesh.colors = {
            Palette::orange, // 0
            Palette::green,  // 1
            Palette::pink,   // 2
            Palette::red,    // 3
            Palette::blue    // 4
        };
        
        // Pyramid: 18 indices
        mesh.indices = {
            2, 1, 0,    // bottom, half1
            0, 3, 2,    // bottom, half2
            0, 4, 3,    // right side face
            3, 4, 2,    // front right side
            2, 4, 1,    // left side face
            1, 4, 0,    // back side face
        };
        
        mesh.faces = {
        {  0, 6 }, // Bottom.
        {  6, 3 }, // Right.
        {  9, 3 }, // Front.
        { 12, 3 }, // Left.
        { 15, 3 }  // Back.
        };
        
        
        // badly? assign UVs based on each vertex's X and Y position, faces perpendicular to this plane get stretched.
        for (AcornData::Vertex& vertex : mesh.vertices)
        {
            vertex.uv = glm::vec2(
                vertex.position.x + 0.5f,
                vertex.position.y + 0.5f
            );
        }
        mesh.hasUVs = true;
        
        return mesh;
    }
    
    
    AcornData::CpuMeshData MakeDefaultPyramid()
    {
        using AcornData::CpuMeshData;

        CpuMeshData mesh;
        
        // 1. Positions, grouped by face.
        mesh.vertices = {
            // Bottom: vertices 0-3
            { glm::vec3( 0.5f, -0.5f, -0.5f) }, // 0: Back right
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 1: Front right
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 2: Front left
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 3: Back left

            // Right: vertices 4-6
            { glm::vec3( 0.5f, -0.5f, -0.5f) }, // 4: Back right
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 5: Tip
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 6: Front right

            // Front: vertices 7-9
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 7: Front right
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 8: Tip
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 9: Front left

            // Left: vertices 10-12
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 10: Front left
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 11: Tip
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 12: Back left

            // Back: vertices 13-15
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 13: Back left
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 14: Tip
            { glm::vec3( 0.5f, -0.5f, -0.5f) }  // 15: Back right
        };
        
        // 2. One color per vertex.
        mesh.colors = {
            // Bottom: vertices 0-3
                { Palette::orange },    // 0: Back right
                { Palette::red },       // 1: Front right
                { Palette::pink },      // 2: Front left
                { Palette::green },     // 3: Back left

                // Right: vertices 4-6
                { Palette::orange },    // 4: Back right
                { Palette::blue },      // 5: Tip
                { Palette::red },       // 6: Front right

                // Front: vertices 7-9
                { Palette::red },       // 7: Front right
                { Palette::blue },      // 8: Tip
                { Palette::pink },      // 9: Front left

                // Left: vertices 10-12
                { Palette::pink },      // 10: Front left
                { Palette::blue },      // 11: Tip
                { Palette::green },     // 12: Back left

                // Back: vertices 13-15
                { Palette::green },     // 13: Back left
                { Palette::blue },      // 14: Tip
                { Palette::orange }     // 15: Back right
        };
        
        
        // 3. Indices, grouped into triangles.
        mesh.indices = {
             0,  1,  2,   2,  3,  0, // Bottom: two triangles
             4,  5,  6,              // Right
             7,  8,  9,              // Front
            10, 11, 12,              // Left
            13, 14, 15               // Back
        };

        // 4. Where each face starts in mesh.indices, and how many indices it uses.
        mesh.faces = {
            { 0, 6 }, // Bottom
            { 6, 3 }, // Right
            { 9, 3 }, // Front
            {12, 3 }, // Left
            {15, 3 }  // Back
        };

        // 5. One normal per face, in the same face order.
        const glm::vec3 faceNormals[5] = {
            { 0.0f,       -1.0f,        0.0f      }, // Bottom: down
            { 0.8944272f,  0.4472136f,  0.0f      }, // Right: +X and up
            { 0.0f,        0.4472136f,  0.8944272f}, // Front: +Z and up
            {-0.8944272f,  0.4472136f,  0.0f      }, // Left: -X and up
            { 0.0f,        0.4472136f, -0.8944272f}  // Back: -Z and up
        };

        // 6. UVs for the square bottom and the triangular sides.
        const glm::vec2 bottomUVs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        const glm::vec2 sideUVs[3] = {
            {0.0f, 0.0f}, // First corner on the base edge
            {0.5f, 1.0f}, // Tip
            {1.0f, 0.0f}  // Last corner on the base edge
        };

        // 7. Assign UVs and normals to the four bottom vertices.
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            auto& vertex = mesh.vertices[corner];
            vertex.uv = bottomUVs[corner];
            vertex.normal = faceNormals[0];
        }
        
        // 8. Assign UVs and normals to the three vertices of each side.
        const std::size_t firstSideVertex[4] = {4, 7, 10, 13};

        for (std::size_t side = 0; side < 4; ++side)
        {
            for (std::size_t corner = 0; corner < 3; ++corner)
            {
                auto& vertex = mesh.vertices[firstSideVertex[side] + corner];
                vertex.uv = sideUVs[corner];
                vertex.normal = faceNormals[side + 1];
            }
        }

        mesh.hasUVs = true;
        mesh.hasNormals = true;

        return mesh;
    }
    
    
    AcornData::CpuMeshData MakeCube_Basic()
    {
        using AcornData::CpuMeshData;  
        
        CpuMeshData mesh;
        
        mesh.vertices = {
            { glm::vec3( 0.5f, -0.5f, -0.5f)},  // 0, bottom, back right 
            { glm::vec3(-0.5f, -0.5f, -0.5f)},  // 1, bottom, back left 
            { glm::vec3(-0.5f, -0.5f, 0.5f)},   // 2, bottom, forward left 
            { glm::vec3( 0.5f, -0.5f, 0.5f)},   // 3, bottom, forward right

            { glm::vec3( 0.5f, 0.5f, -0.5f)},   // 4, up, back right 
            { glm::vec3(-0.5f, 0.5f, -0.5f)},   // 5, up, back left 
            { glm::vec3(-0.5f, 0.5f, 0.5f)},    // 6, up, forward left 
            { glm::vec3( 0.5f, 0.5f, 0.5f)}     // 7, up, forward right
        };
        
        mesh.colors = {
            Palette::orange, // 0
            Palette::green,  // 1
            Palette::pink,   // 2
            Palette::red,    // 3

            Palette::orange, // 4 
            Palette::green,  // 5
            Palette::pink,   // 6
            Palette::red     // 7
        };
        
        // Cube: 36 indices
        mesh.indices = {
            2, 1, 0,    // bottom face, half1
            0, 3, 2,    // bottom face, half2
            7, 3, 0,    // right side face, half1
            0, 4, 7,    // right side face, half2
            6, 2, 3,    // front right side, half1
            3, 7, 6,    // front right side, half1
            5, 1, 2,    // left side face, half1
            2, 6, 5,    // left side face, half2
            5, 4, 0,    // back side face, half1
            0, 1, 5,    // back side face, half2
            6, 7, 4,    // top side face, half1
            4, 5, 6,    // top side face, half2
        };
        
        mesh.faces = {
        {  0, 6 },  // Bottom.
        {  6, 6 },  // Right.
        { 12, 6 },  // Front.
        { 18, 6 },  // Left.
        { 24, 6 },  // Back.
        { 30, 6 }   // Top.
        };
        
        
        // badly? assign UVs based on each vertex's X and Y position, faces perpendicular to this plane get stretched.
        for (AcornData::Vertex& vertex : mesh.vertices)
        {
            vertex.uv = glm::vec2(
                vertex.position.x + 0.5f,
                vertex.position.y + 0.5f
            );
        }
        mesh.hasUVs = true;
        
        return mesh; 
    }
    
    
    AcornData::CpuMeshData MakeDefaultCube()
    {
        using AcornData::CpuMeshData;  
        
        CpuMeshData mesh;
        
        mesh.vertices = {
            // front face
            { glm::vec3(-0.5f, -0.5f, 0.5f)},
            { glm::vec3( 0.5f, -0.5f, 0.5f)},
            { glm::vec3( 0.5f,  0.5f, 0.5f)},
            { glm::vec3(-0.5f,  0.5f, 0.5f)},
        
            // Back face
            { glm::vec3(-0.5f, -0.5f, -0.5f)},
            { glm::vec3(-0.5f,  0.5f, -0.5f)},
            { glm::vec3( 0.5f,  0.5f, -0.5f)},
            { glm::vec3( 0.5f, -0.5f, -0.5f)},
        
            // Bottom face
            { glm::vec3(-0.5f, -0.5f, -0.5f)},
            { glm::vec3( 0.5f, -0.5f, -0.5f)},
            { glm::vec3( 0.5f, -0.5f,  0.5f)},
            { glm::vec3(-0.5f, -0.5f,  0.5f)},
        
            //Top face
            { glm::vec3(-0.5f, 0.5f,  -0.5f)},
            { glm::vec3(-0.5f, 0.5f,   0.5f)},
            { glm::vec3( 0.5f, 0.5f,   0.5f)},
            { glm::vec3( 0.5f, 0.5f,  -0.5f)},
        
            // Left face
            { glm::vec3(-0.5f, -0.5f, -0.5f)},
            { glm::vec3(-0.5f, -0.5f,  0.5f)},
            { glm::vec3(-0.5f,  0.5f,  0.5f)},
            { glm::vec3(-0.5f,  0.5f, -0.5f)},
        
            // Right face
            { glm::vec3(0.5f, -0.5f, -0.5f)},
            { glm::vec3(0.5f,  0.5f, -0.5f)},
            { glm::vec3(0.5f,  0.5f,  0.5f)},
            { glm::vec3(0.5f, -0.5f,  0.5f)},
        };
        
        // One color per vertex.
        mesh.colors = {
            // front face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
        
            // Back face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
        
            // Bottom face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
        
            //Top face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
        
            // Left face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
        
            // Right face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
        };
        
        // Cube: 36 indices
        mesh.indices = {
            // Front face
            0,  1,  2,   2,  3,  0,
           // Back face
            4,  5,  6,   6,  7,  4,
           // Bottom face
            8, 9,  10,  10,  11, 8,
           // Top face
           12, 13, 14,  14, 15, 12,
           // Left face
           16, 17, 18,  18, 19, 16,
           // Right face
           20, 21, 22,  22, 23, 20
        };
        
        // Where each face starts in mesh.indices, and how many indices it uses.
        mesh.faces = {
        {  0, 6 }, // Front.
        {  6, 6 }, // Back.
        { 12, 6 }, // Bottom.
        { 18, 6 }, // Top.
        { 24, 6 }, // Left.
        { 30, 6 }  // Right.
        };
        
        // One normal per face, in the same face order. 
        const glm::vec3 faceNormals[6] = {
            { 0.0f,  0.0f,  1.0f}, // Front
            { 0.0f,  0.0f, -1.0f}, // Back
            { 0.0f, -1.0f,  0.0f}, // Bottom
            { 0.0f,  1.0f,  0.0f}, // Top
            {-1.0f,  0.0f,  0.0f}, // Left
            { 1.0f,  0.0f,  0.0f}  // Right
        };
        
        
        const glm::vec2 faceUVs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        for (std::size_t face = 0; face < 6; ++face)
        {
            for (std::size_t corner = 0; corner < 4; ++corner)
            {
                auto& vertex = mesh.vertices[face * 4 + corner];
                vertex.uv = faceUVs[corner];
                vertex.normal = faceNormals[face];
            }
        }

        mesh.hasUVs = true;
        mesh.hasNormals = true;
        
        return mesh; 
    }
    
    
    
    AcornData::CpuMeshData MakeBasicSphere()
    {
        return generateSortofSphere_Color();
    }
    
}



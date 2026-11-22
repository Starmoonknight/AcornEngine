#include "assets/AssimpLoader.h"


// Standard library
#include <iostream>     // std::cout and std::cerr for diagnostics
#include <utility>      // std::move when finishing a mesh
#include <vector>      

// Third-party libraries - assimp 
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>              // Assimp::Importer and ReadFile()
#include <assimp/postprocess.h>             // Triangulation and UV flip flags
#include <assimp/scene.h>                   // aiScene, aiMesh, and aiFace




namespace 
{
    
    AcornData::CpuMeshData ConvertMesh_Assimp(const aiMesh& source)
    {
        AcornData::CpuMeshData mesh;
        mesh.hasNormals = source.HasNormals();
        mesh.hasUVs = source.HasTextureCoords(0);

        mesh.vertices.reserve(source.mNumVertices);
        
        
        for (unsigned int i = 0; i < source.mNumVertices; ++i)
        {
            AcornData::Vertex vertex{};
            
            // positions
            const auto& position = source.mVertices[i];
            vertex.position = glm::vec3(position.x, position.y, position.z);
            // normals
            if (mesh.hasNormals)
            {
                const auto& normal = source.mNormals[i];
                vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
            }
            // texture coordinates
            if (mesh.hasUVs)
            {
                const auto& uv = source.mTextureCoords[0][i];
                vertex.uv = glm::vec2(uv.x, uv.y);
            }
            
            mesh.vertices.push_back(vertex); 
        }
        
        for(unsigned int i = 0; i < source.mNumFaces; ++i)
        {
            const aiFace& face = source.mFaces[i];
            
            // should only accept triangles here, no lines or point  
            if (face.mNumIndices != 3)
            {
                continue;
            }
            
            
            // Record this as one face range, because quads are triangulated they are recorded as two faces. 
            mesh.faces.push_back({mesh.indices.size(), face.mNumIndices});
            
            
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
            {                
                mesh.indices.push_back(face.mIndices[j]);        
            }
        }
        
        return mesh; 
    }
    
}





namespace OBJ_Loader_Assimp
{
    
    std::vector<AcornData::CpuMeshData> LoadMesh_Assimp(const char* filePath, bool flipUVs)
    {
        Assimp::Importer importer;
        
        unsigned int flags = aiProcess_Triangulate;
        if (flipUVs)
        {
            flags |= aiProcess_FlipUVs;
        }
        
        const aiScene* scene = importer.ReadFile(filePath, flags);
        
        
        if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
        {
            std::cerr << "[ERROR::ASSIMP] Could not import " << filePath << ": " 
                      << importer.GetErrorString() << std::endl;
            return {};
        }
        
        std::vector<AcornData::CpuMeshData> meshes;
        
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            AcornData::CpuMeshData mesh = ConvertMesh_Assimp(*scene->mMeshes[i]);

            if (!mesh.indices.empty())
            {
                meshes.push_back(std::move(mesh));
            }
        }
        
        if (meshes.empty())
        {
            std::cerr << "No triangle meshes found in " << filePath << '\n';
        }

        return meshes;        
    }
    
    
}


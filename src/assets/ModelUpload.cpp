#include "ModelUpload.h"

// Project headers
#include "ObjLoader.h"

// Standard library
#include <filesystem>
#include <iostream>

// Third-party libraries
#include "AssimpLoader.h"



namespace OBJ_Loader_Assimp
{
    
    // Loads a model with Assimp, logs the vertex and triangle counts for each mesh,
    // and uploads the meshes as a GpuModel. Requires a current OpenGL context.
    // If flipUVs is true, flips the V texture coordinate during import.
    AcornData::GpuModel UploadModel_Assimp(const char* filePath, bool flipUVs, bool verboseLogs)
    {
        // extra check because file had problems loading earlier
        std::cout << "Working directory: "
                  << std::filesystem::current_path() << '\n';
        std::cout << "Looking for: "
                  << std::filesystem::absolute(filePath) << '\n';
        std::cout << "File exists: "
                  << std::boolalpha << std::filesystem::exists(filePath) << '\n';
    
        // reading all data and translating to the format my program use 
        const std::vector<AcornData::CpuMeshData> meshes = LoadMesh_Assimp(filePath, flipUVs);
        // extra info on what info was read: 
        if (verboseLogs)
        {
            for (const auto& mesh : meshes)
            {
                std::cout << "Model part: " << mesh.vertices.size()
                          << " vertices, " << mesh.indices.size() / 3
                          << " triangles\n";
            }            
        }
        
        // creating a model from the data 
        return Rendering::CreateGpuModel(meshes);
    }
    
}



namespace OBJ_Loader
{
    
    // Loads an OBJ with the custom parser, logs the vertex and triangle counts for
    // each mesh, and uploads the meshes as a GpuModel. Requires a current OpenGL context.
    // If flipUVs is true, flips the V texture coordinate during import.
    // If skipInvalidFaces is true, logs and skips rejected faces. Otherwise, aborts the import when a face is rejected.
    AcornData::GpuModel UploadModel(const char* filePath, bool flipUVs, bool skipInvalidFaces, bool verboseLogs)
    {
        // Show where the program resolves the path when diagnosing missing assets.
        std::cout << "Working directory: "
                  << std::filesystem::current_path() << '\n';
        std::cout << "Looking for: "
                  << std::filesystem::absolute(filePath) << '\n';
        std::cout << "File exists: "
                  << std::boolalpha << std::filesystem::exists(filePath) << '\n';
        
        // reading all data and translating to the format my program use 
        const std::vector<AcornData::CpuMeshData> meshes = LoadMesh(filePath, flipUVs, skipInvalidFaces);
        // extra info on what info was read: 
        if (verboseLogs)
        {
            for (const auto& mesh : meshes)
            {
                std::cout << "Model part: " << mesh.vertices.size()
                          << " vertices, " << mesh.indices.size() / 3
                          << " triangles\n";
            }            
        }
        
        // creating model from the data 
        return Rendering::CreateGpuModel(meshes);
    }
    
}


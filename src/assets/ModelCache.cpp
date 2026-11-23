#include "ModelCache.h"

// Project headers
#include "ObjLoader.h"
#include "AssimpLoader.h"

// Standard library
#include <filesystem>
#include <iostream>
#include <system_error>
#include <utility>
#include <vector>




namespace Assets
{

    // Takes a path parameter and constructs the projectRoot_ member from it.
    ModelCache::ModelCache(std::filesystem::path projectRoot)
        : projectRoot_(std::move(projectRoot))
    {
    }
    


    const AcornData::GpuModel* ModelCache::GetOrLoad(
        const ModelSource& source, 
        bool verboseLogs)
    {
        // Copy the settings so normalization does not modify the
        // caller's saved source description.
        ModelImportSettings settings = source.importSettings;
        
        // Reject an importer value this implementation does not support.
        if (settings.importer != EModelImporter::Assimp &&
            settings.importer != EModelImporter::CustomObj)
        {
            std::cerr << "[ModelCache] Invalid importer\n";
            return nullptr;
        }
        
        // Assimp does not use the custom loader's skip policy.
        // Ignore any irrelevant settings when comparing requests.
        if (settings.importer == EModelImporter::Assimp)
        {
            settings.skipInvalidFaces = false;
        }
        
        
        // Keeping the old load model function logs, but only when verbose is true
        const std::filesystem::path requestedPath = 
            projectRoot_ / source.filePath;

        if (verboseLogs)
        {
            // Use an error-code overload so the diagnostic check itself
            // does not throw for an ordinary filesystem query failure.
            std::error_code existsError;
            const bool fileExists =
                std::filesystem::exists(requestedPath, existsError);

            std::cout
                << "[ModelCache] Working directory: "
                << std::filesystem::current_path() << '\n'
                << "[ModelCache] Source path: "
                << source.filePath << '\n'
                << "[ModelCache] Looking for: "
                << requestedPath << '\n'
                << "[ModelCache] File exists: "
                << (fileExists ? "true" : "false") << '\n';

            if (existsError)
            {
                std::cout
                    << "[ModelCache] Existence check failed: "
                    << existsError.message() << '\n';
            }
        }
        
        
        // Resolve against the fixed project root.
        // canonical removes dot/dot-dot components and resolves symlinks.
        // It reports an error if the source path does not exist.
        std::error_code error;
        
        const std::filesystem::path resolvedPath =
                std::filesystem::canonical(requestedPath, error);

        if (error)
        {
            std::cerr
                << "[ModelCache] Cannot resolve "
                << source.filePath
                << ": " << error.message() << '\n';

            return nullptr;
        }
        
        // Reuse an existing upload with the same path and settings.
        for (const Entry& entry : entries_)
        {
            if (entry.resolvedPath == resolvedPath &&
                entry.settings == settings)
            {
                if (verboseLogs)
                {
                    std::cout
                        << "[ModelCache] Reuse "
                        << resolvedPath << '\n';
                }

                return &entry.model;
            }
        }
        
        // No matching entry exists: perform the actual import/upload.
        if (verboseLogs)
        {
            std::cout
                << "[ModelCache] Load "
                << resolvedPath << '\n';
        }
        
        // Keep the string alive while its c_str() is passed to a loader.
        const std::string pathText = resolvedPath.string();
        
        // Import CPU geometry using the selected loader.
        std::vector<AcornData::CpuMeshData> meshes;
        
        // Load the model using the correct importer, with correct settings for that importer type 
        if (settings.importer == EModelImporter::Assimp)
        {
            meshes = OBJ_Loader_Assimp::LoadMesh_Assimp(
                pathText.c_str(),
                settings.flipUVs);
        }
        else
        {
            meshes = OBJ_Loader::LoadMesh(
                pathText.c_str(),
                settings.flipUVs,
                settings.skipInvalidFaces);
        }
        
        // Current import failures produce an empty model.
        // So do not store any empty result as a successfully loaded asset.
        if (meshes.empty())
        {
            std::cerr
                << "[ModelCache] Import returned no meshes: "
                << resolvedPath << '\n';

            return nullptr;
        }
        
        if (verboseLogs)
        {
            for (const auto& mesh : meshes)
            {
                std::cout
                    << "Model part: " << mesh.vertices.size()
                    << " vertices, " << mesh.indices.size() / 3
                    << " triangles\n";
            }
        }
        
        // Upload once, then transfer ownership into the cache.
        AcornData::GpuModel uploaded =  Rendering::CreateGpuModel(meshes);
        
        // Transfer ownership from the local variable into a cache entry.
        // Note: A GpuModel can only have one owner and does not allow copying, 
        //       moving it removes all data from the old owner. 
        entries_.push_back(Entry{
            resolvedPath,
            settings,
            std::move(uploaded)
        });

        // Return the address of the model now owned by the cache.
        return &entries_.back().model;
    }



    void ModelCache::Clear()
    {
        // Destroy entries and their owned GPU models.
        // Every previously returned model pointer becomes invalid.
        entries_.clear();
    }


    
}




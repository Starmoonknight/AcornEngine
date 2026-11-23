#pragma once

// Project headers
#include "rendering/GpuModel.h"

// Standard library
#include <deque>
#include <filesystem>
#include <string>


// Note: remember to add default gray color to loaded models until material handling is added 


namespace Assets
{
    
    enum class EModelImporter
    {
        CustomObj,
        Assimp
    };
    
    
    struct ModelImportSettings
    {
        EModelImporter importer = EModelImporter::Assimp;
        bool flipUVs = false;
        
        // Only used by the custom OBJ importer.
        bool skipInvalidFaces = false;

        // Compare every member when checking for a matching request.
        // The compiler generates the comparison in C++20.
        bool operator==(const ModelImportSettings&) const = default;
    };

    
    struct ModelSource
    {
        std::string filePath;
        //std::filesystem::path filePath;       why not use this here to? 
        ModelImportSettings importSettings{};
    };
    
    
    
    struct ModelCache
    {
        // projectRoot must be an absolute directory.
        // Relative model paths are resolved against this fixed directory.
        explicit ModelCache(std::filesystem::path projectRoot);
        
        // Destroying the entries automatically destroys their GPU models.
        // A suitable OpenGL context must still exist.
        ~ModelCache() = default;
        
        // This owner must remain at a stable location.
        ModelCache(const ModelCache&) = delete;
        ModelCache& operator=(const ModelCache&) = delete;
        ModelCache(ModelCache&&) = delete;
        ModelCache& operator=(ModelCache&&) = delete;
        
        
        // Return borrowed geometry for this source.
        // On a miss, import and upload it before storing the owner.
        // Return nullptr for reported path/import failures.
        const AcornData::GpuModel* GetOrLoad(
            const ModelSource& source,
            bool verboseLogs = false);

        
        // Release all cached geometry.
        // All instances using it must already be destroyed or detached.
        // Requires a suitable current OpenGL context.
        void Clear();
        
        
    private:
        
        struct Entry
        {
            std::filesystem::path resolvedPath;
            ModelImportSettings settings;
            AcornData::GpuModel model;
        };
        
        std::filesystem::path projectRoot_;
        
        // Append-only between Clear calls.
        // Appending preserves addresses of the existing entries.
        // std::deque stores segmented memory in non-sequential way managed by an internal 
        // pointer map of fixed-size contiguous chunks rather than a single contiguous array.
        std::deque<Entry> entries_;
    };
    
}




#pragma once


// Project headers
#include "geometry/CpuMeshData.h" 

// Standard library
#include <vector>       // Mesh data and temporary OBJ attribute lists



namespace OBJ_Loader
{
    
    // Reads an OBJ file into CPU-side meshes, using `o` lines as mesh boundaries.
    // Optionally flips the vertical UV coordinate. Returns an empty vector on failure.
    // If skipInvalidFaces is true, log and skip rejected faces. Otherwise, abort the import.
    std::vector<AcornData::CpuMeshData> LoadMesh(
        const char* filePath, 
        bool flipUVs = false, bool 
        skipInvalidFaces = false);
    
    
}


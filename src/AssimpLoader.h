#pragma once


// Project headers
#include "CpuMeshData.h" 

// Standard library
#include <vector>       // Mesh data and temporary OBJ attribute lists


namespace OBJ_Loader_Assimp
{
    
    std::vector<AcornData::CpuMeshData> LoadMesh_Assimp(
        const char* filePath, 
        bool flipUVs = false);
    
    
}



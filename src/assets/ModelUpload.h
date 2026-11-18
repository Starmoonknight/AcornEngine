#pragma once

// Project headers
#include "rendering/GpuModel.h"



namespace OBJ_Loader_Assimp
{
    
    // Loads a model with Assimp, logs the vertex and triangle counts for each mesh,
    // and uploads the meshes as a GpuModel. Requires a current OpenGL context.
    // If flipUVs is true, flips the V texture coordinate during import.
    AcornData::GpuModel UploadModel_Assimp(const char* filePath, bool flipUVs = false, bool verboseLogs = false);
    
}



namespace OBJ_Loader
{
    
    // Loads an OBJ with the custom parser, logs the vertex and triangle counts for
    // each mesh, and uploads the meshes as a GpuModel. Requires a current OpenGL context.
    // If flipUVs is true, flips the V texture coordinate during import.
    // If skipInvalidFaces is true, logs and skips rejected faces. Otherwise, aborts the import when a face is rejected.
    AcornData::GpuModel UploadModel(const char* filePath, bool flipUVs = false, bool skipInvalidFaces = false, bool verboseLogs = false);
    
}



#pragma once

// Project headers
#include "SceneManager.h"

// Standard library 
#include <filesystem>
#include <string>




/*
        # Acorn scene format, version 1
        version 1
        scene "Demo 1"
        id 0
        background 0.15 0.2 0.25
        split vertical

        # player index, position XYZ, rotation XYZ in degrees
        player 0 -2 1 8 0 0 0
        player 1  2 1 8 0 0 0

        goal 5 0 -5

        # Each object starts a new placed-model description.
        # Following object properties apply to the most recent object.
        object "models/cube/cube.obj"
        position -3 0 0
        rotation 0 0 0
        scale 1 1 1
        color 0.9 0.2 0.2

        object "models/cube/cube.obj"
        position 3 0 0
        color 0.2 0.8 0.3

        # Visible marker: keep its position equal to the goal above. They are not really connected in any way.  
        object "models/cube/cube.obj"
        position 5 0 -5
        scale 0.3 0.3 0.3
        color 1 1 0



        texture ""                   # Empty means no texture path
        import assimp 0 0            # importer, flipUVs, skipInvalidFaces
        flags 1 1 0 0                # visible, overrideColor, textureExpected, lighting
        tint 1 0                     # tintAlpha, tintTextureAlpha



        NOTE: Only supports positive model scales 

*/


namespace SceneFile
{
    
    
    // Parses descriptions only; does not create GPU resources.
    // On failure, output is unchanged and error explains the problem.
    bool Read(
        const std::filesystem::path& path,
        AcornData::SceneDescription& output,
        std::string& error);

    
    // Saves starting scene data, not a snapshot of live player state.
    // The destination's parent directory must already exist.
    bool Write(
        const std::filesystem::path& path,
        const AcornData::SceneDescription& description,
        std::string& error);
    
    
}
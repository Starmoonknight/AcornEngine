#pragma once


// Standard library
#include <string>

// Third-party libraries
#include <flecs.h> 
#include <glad/glad.h>                  // GLuint,  Keep glad include before GLFW include 


namespace AcornData
{
    
    struct TextureId
    {
        // Flecs entity containing Texture asset.
        // Zero means no model asset is assigned.
        flecs::entity_t value = 0;
    };
    
    
    // Shared texture metadata stored in Flecs.
    // objectId is borrowed: main currently owns and deletes the GL texture.
    struct Texture 
    {
        GLuint objectId  = 0;
        std::string path; 
        
        // is this needed? 
        //int width = 0;
        //int height = 0;
    };      
    
    
    
}



namespace Assets
{
    // Load an image and upload an OpenGL 2D texture.
    // Returns zero when image loading fails.
    // Caller owns the returned GL object and must delete it while
    // a suitable OpenGL context is current.
    //
    // Uses the current stb_image vertical-flip setting.
    // Changes the active texture unit's GL_TEXTURE_2D binding.
    GLuint LoadTexture2D(const char* filePath); 
    
    
}
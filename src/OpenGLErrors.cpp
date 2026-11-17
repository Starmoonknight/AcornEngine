#include "OpenGLErrors.h"

// Standard library
#include <iostream>             // std::cout and std::cerr for diagnostics

// Third-party libraries
#include <glad/glad.h>          // Keep glad include before GLFW include 



namespace 
{
    
    // some common error types as text 
    const char* OpenGLErrorName(GLenum error)
    {
        switch (error)
        {
        case GL_INVALID_ENUM:      return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:     return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:     return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default:
            return "Unknown OpenGL error";
        }
    }
    
}

    
// gets OpenGl errors, manual check
void CheckOpenGLErrors(const char* stage)
{
    // returns and clears one pending error flag
    GLenum error = glGetError();

    // print every error together with the context stage text  
    while (error != GL_NO_ERROR)
    {
        std::cerr << "[OpenGL][" << stage << "] "
              << OpenGLErrorName(error)
              << " (" << error << ")\n";
            
        error = glGetError();
    }
}
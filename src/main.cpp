// Project headers
#include "ShaderProgram.h" 

// Standard Library
#include <cassert>          // assert
#include <type_traits>      // std::is_standard_layout
#include <limits>           // std::numeric_limits
#include <utility>
#include <cmath>
#include <vector>
#include <cstddef>
#include <iostream>
#include <filesystem>

// Third-party libraries - OpenGL
#include <glad/glad.h>                  // glad include should be before GLFW include 
#include <glfw/glfw3.h>

// Third-party libraries - GLM
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Third-party libraries - stb 
#include "stb_image.h"

// Third-party libraries - assimp 
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>



namespace
{
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    
    constexpr float nearPlane = 0.1f;
    constexpr float farPlane = 100.0f;
    
    constexpr glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    
    
    
    // gets GLFW errors, automatic
    void glfwErrorCallback(int error, const char* description)
    {
        std::cerr << "GLFW error (" << error << "): " << description << '\n';
    }
    
    /*
    // gets GLFW errors, automatic      (not in use, just looking at setup)
    void APIENTRY OpenGLDebugCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam)
    {
        std::cerr << "[OpenGL]["
                  << (type == GL_DEBUG_TYPE_ERROR ? "Error" : "Debug")
                  << "][ID " << id << "] "
                  << message << '\n';
    }
    */
    
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
    
    
    void framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height); 
    }
    
    
    bool InitializeProgram()
    {
        // link function GLFW calls when error happens, 
        // set before glfwInit lets it report initialization errors 
        glfwSetErrorCallback(glfwErrorCallback);
        
        if (glfwInit() != GLFW_TRUE)
        {
            std::cout << "Failed to initialize GLFW." << '\n';
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
        // Apple not possible on 4.6    :( 
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    
        return true;
    }
    
    GLFWwindow* CreateWindow()
    {
        /*
        //       (not in use, just looking at setup)
        // setting up a callback on errors for OpenGL, before creating window/context: 
        //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        */
        
        GLFWwindow *window = glfwCreateWindow(WindowWidth, WindowHeight, "Hello World", nullptr, nullptr);
        if (window == nullptr)
        {
            std::cout << "Failed to create GLFW window"  << '\n';
            return nullptr;
        }
        glfwMakeContextCurrent(window);
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD\n";
        
            glfwDestroyWindow(window);
            return nullptr;
        }
        
        /*
        //       (not in use, just looking at setup)
        // setting up a callback on errors for OpenGL, after the context is current and GLAD successfully loaded:
        //glEnable(GL_DEBUG_OUTPUT);
        //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        //glDebugMessageCallback(OpenGLDebugCallback, nullptr);
        */
        
        // will supply the correct framebuffer height automatically and width on window resize 
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
        return window; 
    }
    
    
    void processInput_ESCAPE(GLFWwindow* window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    
    struct GeometryRenderSettings
    {
        bool bPreviousPress_TAB = false;
        bool bwireframe = false;
        
        bool bPreviousPress_CAPS = false;
        bool bCullFace = true;
        
        bool bPreviousPress_Enter = false;
        bool bPerspectiveView = true; 
    }; 
    
    void processInput_RenderSettings(GLFWwindow* window, GeometryRenderSettings& settings)
    {
        bool currentTAB = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (currentTAB && !settings.bPreviousPress_TAB)
        {
            settings.bwireframe = !settings.bwireframe;
            settings.bwireframe == true ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        // Set drawing mode to wireframe
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);        // Set drawing mode to filled, the default. 
        }
        settings.bPreviousPress_TAB = currentTAB;
        
        bool currentCAPS = glfwGetKey(window, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS;
        if (currentCAPS && !settings.bPreviousPress_CAPS)
        {
            settings.bCullFace = (glIsEnabled(GL_CULL_FACE) == GL_TRUE);        // to make sure it is not getting out of sync with the bool meaning 
            settings.bCullFace = !settings.bCullFace;
            
            settings.bCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        }
        settings.bPreviousPress_CAPS = currentCAPS;
        
        bool currentEnter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (currentEnter && !settings.bPreviousPress_Enter)
        {
            settings.bPerspectiveView = !settings.bPerspectiveView;
        }
        settings.bPreviousPress_Enter = currentEnter;
        
    }
    
    
}




namespace Palette
{
    // Colours
    const glm::vec3 red         {1.0f,  0.0f,  0.0f};
    const glm::vec3 magenta     {1.0f,  0.0f,  1.0f};
    const glm::vec3 pink        {1.0f,  0.75f, 0.8f};
    const glm::vec3 coral       {1.0f,  0.5f,  0.31f};   
    const glm::vec3 orange      {1.0f,  0.5f,  0.0f};
    const glm::vec3 yellow      {1.0f,  1.0f,  0.0f};

    const glm::vec3 blue        {0.0f,  0.0f,  1.0f};
    const glm::vec3 navy        {0.0f,  0.0f,  0.5f};
    const glm::vec3 cyan        {0.0f,  1.0f,  1.0f};
    const glm::vec3 teal        {0.0f,  0.5f,  0.5f};
    const glm::vec3 purple      {0.5f,  0.0f,  0.5f};

    const glm::vec3 green       {0.0f,  1.0f,  0.0f};
    const glm::vec3 forestGreen {0.13f, 0.55f, 0.13f};

    const glm::vec3 black       {0.0f,  0.0f,  0.0f};
    const glm::vec3 white       {1.0f,  1.0f,  1.0f};
}


namespace AcornData
{
    struct Transform
    {
        glm::vec3 position{0.0f};
        glm::vec3 rotationDegrees{0.0f};
        glm::vec3 scale{1.0f};
    };
    
    // describes one rendering vertex position plus associated attributes
    struct Vertex
    {        
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f};
        glm::vec2 uv{0.0f};
    };
    static_assert(
        std::is_standard_layout<Vertex>::value,
        "Vertex must support standard-layout member offsets"
    );
    
    enum TextureType
    {
        Diffuse,
        Specular,
        Normal,
    };
    
    struct Texture {
        GLuint id = 0;
        TextureType type;   // type e.g. a diffuse or specular texture. 
    };  
    
    // a set of consecutive indices that makes a full face 
    struct FaceRange
    {
        std::size_t firstIndex = 0;
        std::size_t indexCount = 0;
    };
    
    // CPU-side geometry data
    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices; 
        
        //std::vector<Texture> textures;    // in MeshData or in GpuMesh? 
        std::vector<glm::vec3> colors;          // Empty when this mesh has no per-vertex hard coded colors.
        
        std::vector<FaceRange> faces;        
        
        bool hasNormals = false;
        bool hasUVs = false;
    };
    
    // GPU handles and drawing metadata
    struct GpuMesh
    {
        GLuint vao = 0;
        GLuint vertexVBO = 0;
        GLuint colorVBO = 0;
        GLuint ebo = 0;
        
        GLsizei index_count = 0;
        std::vector<FaceRange> faces;
    };
    
    
    struct RenderObject
    {        
        const GpuMesh* mesh= nullptr;
        Transform transform{};
        
        bool overrideColor = false;
        glm::vec3 solidColor{1.0f};
        
        GLuint textureID = 0; // 0 means no texture selected for this object.
    };
    
}


namespace AcornMath
{
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees)
    {
        // A starting matrix that makes no change.
        const glm::mat4 identity(1.0f);
        
        // Describe the object's size.
        glm::mat4 scaleMatrix = glm::scale(
            identity, 
            scale
        );
        
        // Describe rotation around the X axis.
        glm::mat4 rotationMatrix_X = glm::rotate(
            identity, 
            glm::radians(rotationDegrees.x), 
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
        
        // Describe rotation around the Y axis.
        glm::mat4 rotationMatrix_Y = glm::rotate(
            identity, 
            glm::radians(rotationDegrees.y), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        // Describe rotation around the Z axis.
        glm::mat4 rotationMatrix_Z = glm::rotate(
            identity, 
            glm::radians(rotationDegrees.z), 
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        
        // reads from right to left, rotation needs to be added X, then Y, then Z.
        glm::mat4 rotationMatrix = rotationMatrix_Z * rotationMatrix_Y * rotationMatrix_X; 
        
        // Describe the object's position.
        glm::mat4 translationMatrix = glm::translate(
            identity, 
            position
        );
        
        
        // reads from right to left, applying scale, then rotation, then position.           y=C(B(Ax)) 
        glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        
        return modelMatrix;
    }
    
    glm::mat4 BuildModelMatrix(const AcornData::Transform& transform)
    {
        return BuildModelMatrix(
            transform.position,
            transform.scale,
            transform.rotationDegrees
        );
    }
    
    
}



namespace  BasicMeshes
{
    AcornData::MeshData MakePyramid_Basic()
    {
        using AcornData::MeshData;   
        
        MeshData mesh;
        
        mesh.vertices = {
            { glm::vec3( 0.5f, -0.5f, -0.5f) }, // 0, bottom, back right 
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 1, bottom, back left 
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 2, bottom, front left 
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 3, bottom, front right
            { glm::vec3( 0.0f,  0.5f,  0.0f) }  // 4, top, middle
        };
        
        mesh.colors = {
            Palette::orange, // 0
            Palette::green,  // 1
            Palette::pink,   // 2
            Palette::red,    // 3
            Palette::blue    // 4
        };
        
        // Pyramid: 18 indices
        mesh.indices = {
            2, 1, 0,    // bottom, half1
            0, 3, 2,    // bottom, half2
            0, 4, 3,    // right side face
            3, 4, 2,    // front right side
            2, 4, 1,    // left side face
            1, 4, 0,    // back side face
        };
        
        mesh.faces = {
        {  0, 6 }, // Bottom.
        {  6, 3 }, // Right.
        {  9, 3 }, // Front.
        { 12, 3 }, // Left.
        { 15, 3 }  // Back.
        };
        
        
        // badly? assign UVs based on each vertex's X and Y position, faces perpendicular to this plane get stretched.
        for (AcornData::Vertex& vertex : mesh.vertices)
        {
            vertex.uv = glm::vec2(
                vertex.position.x + 0.5f,
                vertex.position.y + 0.5f
            );
        }
        mesh.hasUVs = true;
        
        return mesh;
    }
    
    
    AcornData::MeshData MakeDefaultPyramid()
    {
        using AcornData::MeshData;

        MeshData mesh;
        
        // 1. Positions, grouped by face.
        mesh.vertices = {
            // Bottom: vertices 0-3
            { glm::vec3( 0.5f, -0.5f, -0.5f) }, // 0: Back right
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 1: Front right
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 2: Front left
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 3: Back left

            // Right: vertices 4-6
            { glm::vec3( 0.5f, -0.5f, -0.5f) }, // 4: Back right
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 5: Tip
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 6: Front right

            // Front: vertices 7-9
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 7: Front right
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 8: Tip
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 9: Front left

            // Left: vertices 10-12
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 10: Front left
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 11: Tip
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 12: Back left

            // Back: vertices 13-15
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 13: Back left
            { glm::vec3( 0.0f,  0.5f,  0.0f) }, // 14: Tip
            { glm::vec3( 0.5f, -0.5f, -0.5f) }  // 15: Back right
        };
        
        // 2. One color per vertex.
        mesh.colors = {
            // Bottom: vertices 0-3
                { Palette::orange },    // 0: Back right
                { Palette::red },       // 1: Front right
                { Palette::pink },      // 2: Front left
                { Palette::green },     // 3: Back left

                // Right: vertices 4-6
                { Palette::orange },    // 4: Back right
                { Palette::blue },      // 5: Tip
                { Palette::red },       // 6: Front right

                // Front: vertices 7-9
                { Palette::red },       // 7: Front right
                { Palette::blue },      // 8: Tip
                { Palette::pink },      // 9: Front left

                // Left: vertices 10-12
                { Palette::pink },      // 10: Front left
                { Palette::blue },      // 11: Tip
                { Palette::green },     // 12: Back left

                // Back: vertices 13-15
                { Palette::green },     // 13: Back left
                { Palette::blue },      // 14: Tip
                { Palette::orange }     // 15: Back right
        };
        
        
        // 3. Indices, grouped into triangles.
        mesh.indices = {
             0,  1,  2,   2,  3,  0, // Bottom: two triangles
             4,  5,  6,              // Right
             7,  8,  9,              // Front
            10, 11, 12,              // Left
            13, 14, 15               // Back
        };

        // 4. Where each face starts in mesh.indices, and how many indices it uses.
        mesh.faces = {
            { 0, 6 }, // Bottom
            { 6, 3 }, // Right
            { 9, 3 }, // Front
            {12, 3 }, // Left
            {15, 3 }  // Back
        };

        // 5. One normal per face, in the same face order.
        const glm::vec3 faceNormals[5] = {
            { 0.0f,       -1.0f,        0.0f      }, // Bottom: down
            { 0.8944272f,  0.4472136f,  0.0f      }, // Right: +X and up
            { 0.0f,        0.4472136f,  0.8944272f}, // Front: +Z and up
            {-0.8944272f,  0.4472136f,  0.0f      }, // Left: -X and up
            { 0.0f,        0.4472136f, -0.8944272f}  // Back: -Z and up
        };

        // 6. UVs for the square bottom and the triangular sides.
        const glm::vec2 bottomUVs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        const glm::vec2 sideUVs[3] = {
            {0.0f, 0.0f}, // First corner on the base edge
            {0.5f, 1.0f}, // Tip
            {1.0f, 0.0f}  // Last corner on the base edge
        };

        // 7. Assign UVs and normals to the four bottom vertices.
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            auto& vertex = mesh.vertices[corner];
            vertex.uv = bottomUVs[corner];
            vertex.normal = faceNormals[0];
        }
        
        // 8. Assign UVs and normals to the three vertices of each side.
        const std::size_t firstSideVertex[4] = {4, 7, 10, 13};

        for (std::size_t side = 0; side < 4; ++side)
        {
            for (std::size_t corner = 0; corner < 3; ++corner)
            {
                auto& vertex = mesh.vertices[firstSideVertex[side] + corner];
                vertex.uv = sideUVs[corner];
                vertex.normal = faceNormals[side + 1];
            }
        }

        mesh.hasUVs = true;
        mesh.hasNormals = true;

        return mesh;
    }
    
    
    AcornData::MeshData MakeCube_Basic()
    {
        using AcornData::MeshData;  
        
        MeshData mesh;
        
        mesh.vertices = {
            { glm::vec3( 0.5f, -0.5f, -0.5f)},  // 0, bottom, back right 
            { glm::vec3(-0.5f, -0.5f, -0.5f)},  // 1, bottom, back left 
            { glm::vec3(-0.5f, -0.5f, 0.5f)},   // 2, bottom, forward left 
            { glm::vec3( 0.5f, -0.5f, 0.5f)},   // 3, bottom, forward right

            { glm::vec3( 0.5f, 0.5f, -0.5f)},   // 4, up, back right 
            { glm::vec3(-0.5f, 0.5f, -0.5f)},   // 5, up, back left 
            { glm::vec3(-0.5f, 0.5f, 0.5f)},    // 6, up, forward left 
            { glm::vec3( 0.5f, 0.5f, 0.5f)}     // 7, up, forward right
        };
        
        mesh.colors = {
            Palette::orange, // 0
            Palette::green,  // 1
            Palette::pink,   // 2
            Palette::red,    // 3

            Palette::orange, // 4 
            Palette::green,  // 5
            Palette::pink,   // 6
            Palette::red     // 7
        };
        
        // Cube: 36 indices
        mesh.indices = {
            2, 1, 0,    // bottom face, half1
            0, 3, 2,    // bottom face, half2
            7, 3, 0,    // right side face, half1
            0, 4, 7,    // right side face, half2
            6, 2, 3,    // front right side, half1
            3, 7, 6,    // front right side, half1
            5, 1, 2,    // left side face, half1
            2, 6, 5,    // left side face, half2
            5, 4, 0,    // back side face, half1
            0, 1, 5,    // back side face, half2
            6, 7, 4,    // top side face, half1
            4, 5, 6,    // top side face, half2
        };
        
        mesh.faces = {
        {  0, 6 },  // Bottom.
        {  6, 6 },  // Right.
        { 12, 6 },  // Front.
        { 18, 6 },  // Left.
        { 24, 6 },  // Back.
        { 30, 6 }   // Top.
        };
        
        
        // badly? assign UVs based on each vertex's X and Y position, faces perpendicular to this plane get stretched.
        for (AcornData::Vertex& vertex : mesh.vertices)
        {
            vertex.uv = glm::vec2(
                vertex.position.x + 0.5f,
                vertex.position.y + 0.5f
            );
        }
        mesh.hasUVs = true;
        
        return mesh; 
    }
    
    
    AcornData::MeshData MakeDefaultCube()
    {
        using AcornData::MeshData;  
        
        MeshData mesh;
        
        mesh.vertices = {
            // front face
            { glm::vec3(-0.5f, -0.5f, 0.5f)},
            { glm::vec3( 0.5f, -0.5f, 0.5f)},
            { glm::vec3( 0.5f,  0.5f, 0.5f)},
            { glm::vec3(-0.5f,  0.5f, 0.5f)},
        
            // Back face
            { glm::vec3(-0.5f, -0.5f, -0.5f)},
            { glm::vec3(-0.5f,  0.5f, -0.5f)},
            { glm::vec3( 0.5f,  0.5f, -0.5f)},
            { glm::vec3( 0.5f, -0.5f, -0.5f)},
        
            // Bottom face
            { glm::vec3(-0.5f, -0.5f, -0.5f)},
            { glm::vec3( 0.5f, -0.5f, -0.5f)},
            { glm::vec3( 0.5f, -0.5f,  0.5f)},
            { glm::vec3(-0.5f, -0.5f,  0.5f)},
        
            //Top face
            { glm::vec3(-0.5f, 0.5f,  -0.5f)},
            { glm::vec3(-0.5f, 0.5f,   0.5f)},
            { glm::vec3( 0.5f, 0.5f,   0.5f)},
            { glm::vec3( 0.5f, 0.5f,  -0.5f)},
        
            // Left face
            { glm::vec3(-0.5f, -0.5f, -0.5f)},
            { glm::vec3(-0.5f, -0.5f,  0.5f)},
            { glm::vec3(-0.5f,  0.5f,  0.5f)},
            { glm::vec3(-0.5f,  0.5f, -0.5f)},
        
            // Right face
            { glm::vec3(0.5f, -0.5f, -0.5f)},
            { glm::vec3(0.5f,  0.5f, -0.5f)},
            { glm::vec3(0.5f,  0.5f,  0.5f)},
            { glm::vec3(0.5f, -0.5f,  0.5f)},
        };
        
        // One color per vertex.
        mesh.colors = {
            // front face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
        
            // Back face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
        
            // Bottom face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
        
            //Top face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
        
            // Left face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
        
            // Right face
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
        };
        
        // Cube: 36 indices
        mesh.indices = {
            // Front face
            0,  1,  2,   2,  3,  0,
           // Back face
            4,  5,  6,   6,  7,  4,
           // Bottom face
            8, 9,  10,  10,  11, 8,
           // Top face
           12, 13, 14,  14, 15, 12,
           // Left face
           16, 17, 18,  18, 19, 16,
           // Right face
           20, 21, 22,  22, 23, 20
        };
        
        // Where each face starts in mesh.indices, and how many indices it uses.
        mesh.faces = {
        {  0, 6 }, // Front.
        {  6, 6 }, // Back.
        { 12, 6 }, // Bottom.
        { 18, 6 }, // Top.
        { 24, 6 }, // Left.
        { 30, 6 }  // Right.
        };
        
        // One normal per face, in the same face order. 
        const glm::vec3 faceNormals[6] = {
            { 0.0f,  0.0f,  1.0f}, // Front
            { 0.0f,  0.0f, -1.0f}, // Back
            { 0.0f, -1.0f,  0.0f}, // Bottom
            { 0.0f,  1.0f,  0.0f}, // Top
            {-1.0f,  0.0f,  0.0f}, // Left
            { 1.0f,  0.0f,  0.0f}  // Right
        };
        
        
        const glm::vec2 faceUVs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        for (std::size_t face = 0; face < 6; ++face)
        {
            for (std::size_t corner = 0; corner < 4; ++corner)
            {
                auto& vertex = mesh.vertices[face * 4 + corner];
                vertex.uv = faceUVs[corner];
                vertex.normal = faceNormals[face];
            }
        }

        mesh.hasUVs = true;
        mesh.hasNormals = true;
        
        return mesh; 
    }
    
    
    
        // It's not the nicest sphere, nor the nicest code, but it sort of works.
    void generateSortofSphere(std::vector<float> &vertices, std::vector<unsigned int> &indices, int rings = 9, int slices = 16) {

        if (rings < 3)  { std::cout << "no sphere generated. rings value < 3"  << std::endl;  return; }
        if (slices < 4) { std::cout << "no sphere generated. slices value < 4" << std::endl;  return; }
        
        float PIE = 3.1415f;
        
        vertices.clear();
        indices.clear();

        float angle_step    = 360.0f / slices; 
        float current_angle = 0.0f;
        
        // Vertices
        float y_step = 2.0f / (rings - 1);
        float y_pos  = 1.0f;

        for (int ring = 0; ring < rings; ring++) {

            int iteration_slices = slices;
            if (ring == 0 || ring == rings - 1) { iteration_slices = 1; }

            for (int i = 0; i < iteration_slices; i++) {

                float radius = sinf(acos(y_pos));

                float x_pos = cosf(current_angle * PIE / 180) * radius;
                float z_pos = sinf(current_angle * PIE / 180) * radius;

                vertices.push_back(x_pos);
                vertices.push_back(y_pos);
                vertices.push_back(z_pos);

                current_angle += angle_step;
            }

            y_pos -= y_step;
            current_angle = 0.0f;
        }

        std::cout << "Sphere vertex count:" << vertices.size() / 3 << std::endl;

        //Indices
        unsigned int top_most_index = 0;
        unsigned int bottom_most_index = vertices.size() / 3 - 1;

        for (int i = 0; i < rings - 1; i++) {

            for (int j = 0; j < slices; j++) {

                unsigned int tl_index = (i - 1) * slices + j + 1;
                unsigned int tr_index = (i - 1) * slices + j + 1 + 1;
                unsigned int bl_index = (i + 0) * slices + j + 1;
                unsigned int br_index = (i + 0) * slices + j + 1 + 1;

                if (j == slices - 1) { // For the seam
                    tr_index = (i - 2) * slices + j + 1 + 1;
                    br_index = (i - 1) * slices + j + 1 + 1;
                }

                if (i == 0) { // First ring

                    indices.push_back(top_most_index);
                    indices.push_back(br_index);
                    indices.push_back(bl_index);

                } else if (i == rings - 2) { // Last ring

                    indices.push_back(tr_index);
                    indices.push_back(bottom_most_index);
                    indices.push_back(tl_index);

                } else {

                    indices.push_back(tl_index);
                    indices.push_back(br_index);
                    indices.push_back(bl_index);

                    indices.push_back(tl_index);
                    indices.push_back(tr_index);
                    indices.push_back(br_index);
                }
            }
        }

        std::cout << "Sphere triangle count:" << indices.size() / 3 << std::endl;
    }
    
    AcornData::MeshData generateSortofSphere_Color(int rings = 9, int slices = 16)
    {
        using AcornData::MeshData;  
        
        MeshData sphereMesh;  
        std::vector<float> spherePositions; 
        
        generateSortofSphere(spherePositions, sphereMesh.indices, rings, slices); 
        
        for (std::size_t i = 0; i < spherePositions.size(); i += 3)
        {
            // Position,    and reducing the generated diameter from 2 to 1.
            AcornData::Vertex vertex;
            vertex.position = glm::vec3(
                spherePositions[i], 
                spherePositions[i + 1],
                spherePositions[i + 2]
            ) * 0.5f;
            
            sphereMesh.vertices.push_back(vertex); 
            
            // Color
            sphereMesh.colors.push_back(Palette::orange);
        }
        
        return sphereMesh; 
    };    
    
    AcornData::MeshData MakeBasicSphere()
    {
        return generateSortofSphere_Color();
    }
    
}


namespace Rendering
{
    
    AcornData::GpuMesh CreateGpuMesh(const AcornData::MeshData& mesh, GLenum usage = GL_STATIC_DRAW)
    {
        using AcornData::Vertex;
        using AcornData::GpuMesh;
        // Colors are absent, or there is one color per vertex.
        assert(mesh.colors.empty() || mesh.colors.size() == mesh.vertices.size());
        
        // Triangle indices come in groups of three.
        assert(mesh.indices.size() % 3 == 0);       
        // Every index refers to an existing vertex.
        for (unsigned int index : mesh.indices)     
        {
            assert(index < mesh.vertices.size());
        }
        // The total index count fits the draw-call parameter.
        assert(mesh.indices.size() <= static_cast<std::size_t>((std::numeric_limits<GLsizei>::max)()));  
        
        
        // ----- Setup -----
        
        GpuMesh gpu; 
        gpu.index_count = static_cast<GLsizei>(mesh.indices.size());
        gpu.faces = mesh.faces;
        
        glGenVertexArrays(1, &gpu.vao);
        glGenBuffers(1, &gpu.vertexVBO);
        glGenBuffers(1, &gpu.ebo);
        glBindVertexArray(gpu.vao);
        
        
        // ----- Vertex VBO -----
        glBindBuffer(GL_ARRAY_BUFFER, gpu.vertexVBO);
        GLsizeiptr verticesSizeBytes = static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex));          // number of elements x bytes per element.
        glBufferData(GL_ARRAY_BUFFER, verticesSizeBytes, mesh.vertices.data(), usage);
        
        // position,        in attribute slot 0
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 
            static_cast<GLsizei>(sizeof(Vertex)), 
            reinterpret_cast<const void*>(offsetof(Vertex, position))
        );
        glEnableVertexAttribArray(0);
        
        // normal,          in attribute slot 2
        if (mesh.hasNormals)
        {
            glVertexAttribPointer(
                1, 3, GL_FLOAT, GL_FALSE, 
                static_cast<GLsizei>(sizeof(Vertex)), 
                reinterpret_cast<const void*>(offsetof(Vertex, normal))
            );
            
            glEnableVertexAttribArray(1);
        }
        else
        {
            glDisableVertexAttribArray(1);
        }
        
        // uv,              in attribute slot 3
        if (mesh.hasUVs)
        {
            glVertexAttribPointer(
                2, 2, GL_FLOAT, GL_FALSE,
                static_cast<GLsizei>(sizeof(Vertex)),
                reinterpret_cast<const void*>(offsetof(Vertex, uv))
            );
            
            glEnableVertexAttribArray(2);
        }
        else
        {
            glDisableVertexAttribArray(2);
        }
        CheckOpenGLErrors("geometry buffer and attribute setup");
        
        
        // ----- Color VBO -----
        if (!mesh.colors.empty())
        {
            glGenBuffers(1, &gpu.colorVBO);
            glBindBuffer(GL_ARRAY_BUFFER, gpu.colorVBO);
            
            // color buffer
            GLsizeiptr colorsSizeBytes = static_cast<GLsizeiptr>(mesh.colors.size() * sizeof(glm::vec3)); 
            glBufferData(GL_ARRAY_BUFFER, colorsSizeBytes, mesh.colors.data(), usage);
            
            // color attribute
            glVertexAttribPointer(
                3, 3, GL_FLOAT, GL_FALSE, 
                static_cast<GLsizei>(sizeof(glm::vec3)), 
                nullptr
            );
            glEnableVertexAttribArray(3);
        }
        else
        {
            glDisableVertexAttribArray(3);
        }
        CheckOpenGLErrors("color buffer and attribute setup");
        
        
        // ----- EBO -----
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
        
        GLsizeiptr indicesSizeBytes = static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(unsigned int)); 
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSizeBytes, mesh.indices.data(), usage);
        CheckOpenGLErrors("EBO buffer setup");
        
        
        // ----- Finish setup -----
        glBindVertexArray(0);
        CheckOpenGLErrors("mesh setup");

        return gpu; 
    }
    
    void DestroyGpuMesh(AcornData::GpuMesh& mesh)
    {
        glDeleteVertexArrays(1, &mesh.vao);

        glDeleteBuffers(1, &mesh.vertexVBO);
        glDeleteBuffers(1, &mesh.colorVBO);
        glDeleteBuffers(1, &mesh.ebo);

        mesh = AcornData::GpuMesh{};
    }
    
    
    void SetBasicShaderUniforms_Placement(
        const BasicUniformLocations& uniforms,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection)
    {
        // updates uniforms on the currently active program using the saved uniform locations 
        glUniformMatrix4fv(uniforms.modelLocation, 1 ,GL_FALSE,  glm::value_ptr(model));
        glUniformMatrix4fv(uniforms.viewLocation, 1 ,GL_FALSE,  glm::value_ptr(view));
        glUniformMatrix4fv(uniforms.projectionLocation, 1 ,GL_FALSE,  glm::value_ptr(projection));
    }
    
    void SetBasicShaderUniforms_Visuals(
        const BasicUniformLocations& uniforms,
        bool colorOverride = false,
        glm::vec3 color = glm::vec3(1.0f),
        float colorAmount = 1.0,
        bool useTexture = false,
        float textureColorTintAmount = 0.0)
    {
        glUniform1i(uniforms.overrideColorLocation, colorOverride);
        glUniform3fv(uniforms.colorLocation, 1, glm::value_ptr(color)); 
        glUniform1f(uniforms.colorAmountLocation, colorAmount);
        
        glUniform1i(uniforms.useTextureLocation, useTexture);
        glUniform1f(uniforms.tintTextureColorAmountLocation, textureColorTintAmount);
    }
    
    void SetBasicShaderUniforms(
        const ShaderProgram& shader,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection,
        bool colorOverride = false,
        glm::vec3 color = glm::vec3(1.0f),
        float colorAmount = 1.0f,
        bool useTexture = false,
        float textureColorTintAmount = 0.0f)
    {
        glUseProgram(shader.ID); 
        const BasicUniformLocations& uniforms = shader.GetBasicUniformLocations();
        
        SetBasicShaderUniforms_Placement(uniforms, model, view, projection);
        SetBasicShaderUniforms_Visuals(uniforms, colorOverride, color, colorAmount, useTexture, textureColorTintAmount); 
        CheckOpenGLErrors("setting geometry Uniform's");
    }
    
    
    void DrawMesh(const AcornData::GpuMesh& mesh)
    {
        assert(mesh.vao != 0);
        
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
        
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing mesh");
    }
    
    void DrawMesh_FaceColors(
        const ShaderProgram& shader, 
        const AcornData::GpuMesh& mesh,  
        const std::vector<glm::vec3>& faceColors)
    {
        assert(mesh.vao != 0);
        assert(mesh.index_count >= 0);
        
        const auto& faces = mesh.faces;
        
        assert(!faces.empty());
        assert(faces.size() == faceColors.size());
        
        const std::size_t totalIndexCount = static_cast<std::size_t>(mesh.index_count);
        
        // Check that each range fits within the uploaded index list.
        for (const AcornData::FaceRange& range : faces)
        {
            assert(range.indexCount > 0);
            assert(range.indexCount % 3 == 0);
            assert(range.firstIndex <= totalIndexCount);
            assert(range.indexCount <= totalIndexCount - range.firstIndex);
        }
        
        
        glUseProgram(shader.ID);
        glBindVertexArray(mesh.vao);
        
        const auto& uniforms = shader.GetBasicUniformLocations();
        
        // This function requires a shader with these active uniforms.
        assert(uniforms.overrideColorLocation != -1);
        assert(uniforms.colorLocation != -1);
        
        glUniform1i(uniforms.overrideColorLocation, GL_TRUE);
        
        for (std::size_t face = 0; face < faces.size(); ++face)
        {
            const AcornData::FaceRange& range = faces[face];
            const std::size_t offsetBytes = range.firstIndex * sizeof(unsigned int);
            
            glUniform3fv(uniforms.colorLocation, 1, glm::value_ptr(faceColors[face]));
            
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.indexCount), GL_UNSIGNED_INT, reinterpret_cast<const void*>(offsetBytes));
        }
        
        glUniform1i(uniforms.overrideColorLocation, GL_FALSE);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing mesh with different face colors");
    }
    
    
}


namespace Texture
{
    GLuint LoadTexture2D(const char* filePath)
    {
        int width = 0;
        int height = 0;
        int channelsInFile = 0;
        
        // Return four bytes per pixel (RGBA), regardless of the file's channels.  Should work with both .jpg and .png 
        unsigned char *pixels = stbi_load(filePath, &width, &height, &channelsInFile, STBI_rgb_alpha);  
        
        if (!pixels)
        {
            std::cerr << "Failed to load texture: " << filePath << '\n';
            return 0;
        }
        
        GLuint textureID = 0;
        glGenTextures(1, &textureID);  
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // load and generate the texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // releases the CPU-side pixel array returned by stbi_load() after glTexImage2D() has consumed the pixels for the OpenGL texture
        stbi_image_free(pixels);

        return textureID;
    }
    
}



namespace OBJ_Loader_Assimp
{
    struct ImportedPart
    {
        AcornData::GpuMesh gpuMesh;
        bool hasNormals;
        bool hasUVs;
    };
    
    
    AcornData::MeshData ConvertMesh(const aiMesh& source)
    {
        AcornData::MeshData mesh;
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
            
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
            {                
                mesh.indices.push_back(face.mIndices[j]);        
            }
        }
        
        return mesh; 
    }
    
    
    std::vector<AcornData::MeshData> LoadModel(const char* filePath)
    {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);	
        
        if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
        {
            std::cerr << "[ERROR::ASSIMP] Could not import " << filePath << ": " 
                      << importer.GetErrorString() << std::endl;
            return {};
        }
        
        std::vector<AcornData::MeshData> meshes;
        
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            AcornData::MeshData mesh = ConvertMesh(*scene->mMeshes[i]);

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



namespace OBJ_Loader
{
    
}





namespace Demo
{   
    namespace TestColors
    {
        const std::vector<glm::vec3> pyramid_FaceColors{
            Palette::pink,    // Bottom
            Palette::yellow,  // Right
            Palette::red,     // Front
            Palette::green,   // Left
            Palette::blue,    // Back
        };
        
        // has to match EBO draw order
        const std::vector<glm::vec3> cube_FaceColors{
            Palette::red,     // Bottom
            Palette::orange,  // Right
            Palette::pink,    // Front
            Palette::green,   // Left
            Palette::blue,    // Back
            Palette::yellow   // Top
        };
    };
    
    struct GeometryRuntimeValues_Inputs
    {
        bool bPreviousPress_Space = false;
        
        // Scale
        bool bPreviousPress_Q = false;
        bool bPreviousPress_W = false;
        bool bPreviousPress_E = false;
        
        // Rotation
        bool bPreviousPress_A = false;
        bool bPreviousPress_S = false;
        bool bPreviousPress_D = false;
        
        // Location
        bool bPreviousPress_Z = false;
        bool bPreviousPress_X = false;
        bool bPreviousPress_C = false;
    };
    
    struct GeometryRuntimeValues_Transforms
    {   
        // Scale
        bool bScaleX = false;
        bool bScaleY = false;
        bool bScaleZ = false;
        
        glm::vec3 scale = glm::vec3(1.0f);
        
        bool bGrowingX = true;
        bool bGrowingY = true;
        bool bGrowingZ = true;
    
        // Rotation
        bool bRotateX = false;
        bool bRotateY = false;
        bool bRotateZ = false;
    
        glm::vec3 rotationDegrees = glm::vec3 (0.0f);
    
        // Location
        bool bTranslateX = false;
        bool bTranslateY = false;
        bool bTranslateZ = false;
        
        glm::vec3 translation = glm::vec3 (0.0f);
        
        bool bPositivMovementX = true; 
        bool bPositivMovementY = true; 
        bool bPositivMovementZ = true; 
    };
    
    void HandleRuntimeInputs(
        GLFWwindow* window, 
        GeometryRuntimeValues_Inputs& inputs, 
        GeometryRuntimeValues_Transforms& values)
    {
        bool current_Space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool bReset = false; 
        
        if (current_Space && !inputs.bPreviousPress_Space)
        {
            bReset = !bReset;
        }
        inputs.bPreviousPress_Space = current_Space;
        
        
        bool current_Q = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
        bool current_W = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool current_E = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        
        bool current_A = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool current_S = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool current_D = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        
        bool current_Z = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        bool current_X = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
        bool current_C = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        
        // scale key inputs
        if (current_Q && !inputs.bPreviousPress_Q)
        {
            values.bScaleX = !values.bScaleX;
        }
        inputs.bPreviousPress_Q = current_Q;
        
        if (current_W && !inputs.bPreviousPress_W)
        {
            values.bScaleY = !values.bScaleY;
        }
        inputs.bPreviousPress_W = current_W;
        
        if (current_E && !inputs.bPreviousPress_E)
        {
            values.bScaleZ = !values.bScaleZ;
        }
        inputs.bPreviousPress_E = current_E;
        
        // rotation key inputs
        if (current_A && !inputs.bPreviousPress_A)
        {
            values.bRotateX = !values.bRotateX;
        }
        inputs.bPreviousPress_A = current_A;
        
        if (current_S && !inputs.bPreviousPress_S)
        {
            values.bRotateY = !values.bRotateY;
        }
        inputs.bPreviousPress_S = current_S;
        
        if (current_D && !inputs.bPreviousPress_D)
        {
            values.bRotateZ = !values.bRotateZ;
        }
        inputs.bPreviousPress_D = current_D;
        
        // translate key inputs
        if (current_Z && !inputs.bPreviousPress_Z)
        {
            values.bTranslateX = !values.bTranslateX;
        }
        inputs.bPreviousPress_Z = current_Z;
        
        if (current_X && !inputs.bPreviousPress_X)
        {
            values.bTranslateY = !values.bTranslateY;
        }
        inputs.bPreviousPress_X = current_X;
        
        if (current_C && !inputs.bPreviousPress_C)
        {
            values.bTranslateZ = !values.bTranslateZ;
        }
        inputs.bPreviousPress_C = current_C;        
        
        
        // reset of scale and rotation 
        if (bReset)
        {
            values.scale = glm::vec3(1.0f);           // (1, 1, 1)
            values.rotationDegrees = glm::vec3(0.0f); // (0, 0, 0)
            values.translation = glm::vec3(0.0f);     // (0, 0, 0)                        
        }
    }
    
    void UpdateTransforms(
    GeometryRuntimeValues_Transforms& values,
    float deltaTime)
    {
        constexpr float scaleChangePerSecond = 1.0f;
        constexpr float maxScale = 1.5f;
        constexpr float minScale = 0.25f;
        
        constexpr float rotationDegreesPerSecond = 90.0f;
        
        constexpr float translarationChangePerSecond = 1.0f;
        constexpr float maxTranslate = 2.0f;
        constexpr float minTranslate = -2.0f;
        
        // scale application
        if (values.bScaleX)
        {
            if (values.bGrowingX)
            {
                values.scale.x = values.scale.x + scaleChangePerSecond * deltaTime;
                if (values.scale.x > maxScale)
                {
                    values.scale.x = maxScale;
                    values.bGrowingX = false; 
                }
            }
            else
            {
                values.scale.x = values.scale.x - scaleChangePerSecond * deltaTime;
                if (values.scale.x < minScale)
                {
                    values.scale.x = minScale;
                    values.bGrowingX = true; 
                }
            }
        }
        if (values.bScaleY)
        {
            if (values.bGrowingY)
            {
                values.scale.y = values.scale.y + scaleChangePerSecond * deltaTime;
                if (values.scale.y > maxScale)
                {
                    values.scale.y = maxScale;
                    values.bGrowingY = false; 
                }
            }
            else
            {
                values.scale.y = values.scale.y - scaleChangePerSecond * deltaTime;
                if (values.scale.y < minScale)
                {
                    values.scale.y = minScale;
                    values.bGrowingY = true; 
                }
            }
        }
        if (values.bScaleZ)
        {
            if (values.bGrowingZ)
            {
                values.scale.z = values.scale.z + scaleChangePerSecond * deltaTime;
                if (values.scale.z > maxScale)
                {
                    values.scale.z = maxScale;
                    values.bGrowingZ= false; 
                }
            }
            else
            {
                values.scale.z = values.scale.z - scaleChangePerSecond * deltaTime;
                if (values.scale.z < minScale)
                {
                    values.scale.z = minScale;
                    values.bGrowingZ = true; 
                }
            }
        }
        
        // rotation application
        if (values.bRotateX)
        {
            values.rotationDegrees.x = std::fmod(
                values.rotationDegrees.x + rotationDegreesPerSecond * deltaTime,
                360.0f
            );
        }
        if (values.bRotateY)
        {
            values.rotationDegrees.y = std::fmod(
                values.rotationDegrees.y + rotationDegreesPerSecond * deltaTime,
                360.0f
            );
        }
        if (values.bRotateZ)
        {
            values.rotationDegrees.z = std::fmod(
                values.rotationDegrees.z + rotationDegreesPerSecond * deltaTime,
                360.0f
            );
        }        
        
        // translation application
        if (values.bTranslateX)
        {
            if (values.bPositivMovementX)
            {
                values.translation.x = values.translation.x + translarationChangePerSecond * deltaTime;
                if (values.translation.x > maxTranslate)
                {
                    values.translation.x = maxTranslate;
                    values.bPositivMovementX = false; 
                }
            }
            else
            {
                values.translation.x = values.translation.x - translarationChangePerSecond * deltaTime;
                if (values.translation.x < minTranslate)
                {
                    values.translation.x = minTranslate;
                    values.bPositivMovementX = true; 
                }
            }
        }
        if (values.bTranslateY)
        {
            if (values.bPositivMovementY)
            {
                values.translation.y = values.translation.y + translarationChangePerSecond * deltaTime;
                if (values.translation.y > maxTranslate)
                {
                    values.translation.y = maxTranslate;
                    values.bPositivMovementY = false; 
                }
            }
            else
            {
                values.translation.y = values.translation.y - translarationChangePerSecond * deltaTime;
                if (values.translation.y < minTranslate)
                {
                    values.translation.y = minTranslate;
                    values.bPositivMovementY = true; 
                }
            }
        }
        if (values.bTranslateZ)
        {
            if (values.bPositivMovementZ)
            {
                values.translation.z = values.translation.z + translarationChangePerSecond * deltaTime;
                if (values.translation.z > maxTranslate)
                {
                    values.translation.z = maxTranslate;
                    values.bPositivMovementZ = false; 
                }
            }
            else
            {
                values.translation.z = values.translation.z - translarationChangePerSecond * deltaTime;
                if (values.translation.z < minTranslate)
                {
                    values.translation.z = minTranslate;
                    values.bPositivMovementZ = true; 
                }
            }
        }
    }
    
    
    struct TestingDefaultShapes 
    {
        using Transform = AcornData::Transform;
        using MeshData = AcornData::MeshData;
        using GpuMesh = AcornData::GpuMesh;
        using RenderObject = AcornData::RenderObject;
        
        
        // controlled by keyboard toggles.
        Demo::GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        Demo::GeometryRuntimeValues_Transforms runtime_Info{};
        // Automatic animations.
        Demo::GeometryRuntimeValues_Transforms cubeAuto{};
        Demo::GeometryRuntimeValues_Transforms sphereAuto{};
        Demo::GeometryRuntimeValues_Transforms pyramidAuto{};
        
        TestingDefaultShapes()
        {
            cubeAuto.bRotateY = true;
            sphereAuto.bTranslateZ = true;
            pyramidAuto.bScaleY = true;
        }
        
        // preparing rendering objects  
        MeshData pyramidData = BasicMeshes::MakePyramid_Basic();
        MeshData cubeData = BasicMeshes::MakeCube_Basic();
        MeshData sphereData = BasicMeshes::MakeBasicSphere();
        
        MeshData pyramidData_v2 = BasicMeshes::MakeDefaultPyramid();
        MeshData cubeData_v2 = BasicMeshes::MakeDefaultCube();
        
        
        GpuMesh pyramidGpu = Rendering::CreateGpuMesh(pyramidData);
        GpuMesh cubeGpu = Rendering::CreateGpuMesh(cubeData);
        GpuMesh sphereGpu = Rendering::CreateGpuMesh(sphereData);
        
        GpuMesh pyramidGp_v2 = Rendering::CreateGpuMesh(pyramidData_v2);
        GpuMesh cubeGpu_v2 = Rendering::CreateGpuMesh(cubeData_v2);
    
    
        // making object instances
        RenderObject basicPyramid_1{&pyramidGpu, Transform{glm::vec3(-4.0f, 0.0f, 1.0f)}};
        RenderObject basicPyramid_2{&pyramidGpu, Transform{glm::vec3(-4.0f, 0.0f, -2.5f), glm::vec3(180.0f, 0.0f, 0.0f)}, true, Palette::blue};
        
        RenderObject pyramid_V1{&pyramidGp_v2, Transform{glm::vec3(-4.0f, 0.0f, -6.0f)}};
        RenderObject pyramid_manual{&pyramidGp_v2, Transform{glm::vec3(0.0f, 0.0f, 4.0f)}};
    
        
        RenderObject basicCube_1{&cubeGpu, Transform{glm::vec3(0.0f, 0.0f, 1.0f)}};
        RenderObject basicCube_2{&cubeGpu, Transform{glm::vec3(0.0f, 0.0f, -2.5f)}, true, Palette::pink};
        
        RenderObject cube_V2{&cubeGpu_v2, Transform{glm::vec3(0.0f, 0.0f, -7.0f)}};
        
        
        RenderObject sphere_1{&sphereGpu, Transform{glm::vec3(3.0f, 0.0f, -1.0f)}};
        RenderObject sphere_2{&sphereGpu, Transform{glm::vec3(4.5f, 0.0f, -2.5f)}, true, Palette::forestGreen};
    };
    
    
    void Update_TestingDefaultShapes(
        GLFWwindow* window, const ShaderProgram& shaderProgram, TestingDefaultShapes& basicShapes,
        float deltaTime, const glm::mat4& view, const glm::mat4& projection, 
        GLuint testTexture_crate)
    {
        
        HandleRuntimeInputs(window, basicShapes.runtime_Inputs, basicShapes.runtime_Info);

        UpdateTransforms(basicShapes.runtime_Info, deltaTime);
        UpdateTransforms(basicShapes.cubeAuto, deltaTime);
        UpdateTransforms(basicShapes.sphereAuto, deltaTime);
        UpdateTransforms(basicShapes.pyramidAuto, deltaTime);
        
        
        // activate program
        shaderProgram.use(); 
        
        const BasicUniformLocations& uniforms = shaderProgram.GetBasicUniformLocations();
        glUniform1i(uniforms.textureLocation, 0); // uTexture2D → unit 0
        
        
        
        glm::vec3 manualPosition = basicShapes.runtime_Info.translation;
        glm::vec3 manualScale = basicShapes.runtime_Info.scale;
        glm::vec3 manualRotationDegrees = basicShapes.runtime_Info.rotationDegrees;
        
        
        // Pyramid: placed on the left.
        glm::mat4 pyramidModel_1 = AcornMath::BuildModelMatrix(
            basicShapes.basicPyramid_1.transform.position,
            basicShapes.basicPyramid_1.transform.scale * basicShapes.pyramidAuto.scale,
            basicShapes.basicPyramid_1.transform.rotationDegrees);
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidModel_1, view, projection, basicShapes.basicPyramid_1.overrideColor, basicShapes.basicPyramid_1.solidColor, 1.0f);        
        Rendering::DrawMesh(*basicShapes.basicPyramid_1.mesh);
        
        glm::mat4 pyramidModel_2 = AcornMath::BuildModelMatrix(
            basicShapes.basicPyramid_2.transform.position,
            basicShapes.basicPyramid_2.transform.scale * basicShapes.pyramidAuto.scale,
            basicShapes.basicPyramid_2.transform.rotationDegrees);        
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidModel_2, view, projection, basicShapes.basicPyramid_2.overrideColor, basicShapes.basicPyramid_2.solidColor, 1.0f);
        Rendering::DrawMesh(*basicShapes.basicPyramid_2.mesh);
        
        
        glm::mat4 pyramidModel_v2 = AcornMath::BuildModelMatrix(
            basicShapes.pyramid_V1.transform.position,
            basicShapes.pyramid_V1.transform.scale * basicShapes.pyramidAuto.scale,
            basicShapes.pyramid_V1.transform.rotationDegrees);
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidModel_v2, view, projection, basicShapes.pyramid_V1.overrideColor, basicShapes.pyramid_V1.solidColor, 1.0f);        
        shaderProgram.SetBool("uUseLighting", true);
        Rendering::DrawMesh(*basicShapes.pyramid_V1.mesh);
        
        
        // manually controlled pyramid
        glm::mat4 pyramidModel_Manual = AcornMath::BuildModelMatrix(
            basicShapes.pyramid_manual.transform.position + manualPosition,
            basicShapes.pyramid_manual.transform.scale * manualScale,
            basicShapes.pyramid_manual.transform.rotationDegrees + manualRotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidModel_Manual, view, projection, basicShapes.pyramid_manual.overrideColor, basicShapes.pyramid_manual.solidColor, 1.0f);
        //Rendering::DrawMesh(*pyramid_manual.mesh);
        Rendering::DrawMesh_FaceColors(shaderProgram, *basicShapes.pyramid_manual.mesh, Demo::TestColors::pyramid_FaceColors); 
        shaderProgram.SetBool("uUseLighting", false);
        
        
        
        // Cube version 1: placed in the middle.
        glm::mat4 cubeModel_1 = AcornMath::BuildModelMatrix(
            basicShapes.basicCube_1.transform.position,
            basicShapes.basicCube_1.transform.scale,
            basicShapes.basicCube_1.transform.rotationDegrees + basicShapes.cubeAuto.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, cubeModel_1, view, projection, basicShapes.basicCube_1.overrideColor, basicShapes.basicCube_1.solidColor, 1.0f);
        Rendering::DrawMesh(*basicShapes.basicCube_1.mesh);
        
        
        glm::mat4 cubeModel_2 = AcornMath::BuildModelMatrix(
            basicShapes.basicCube_2.transform.position,
            basicShapes.basicCube_2.transform.scale,
            basicShapes.basicCube_2.transform.rotationDegrees - basicShapes.cubeAuto.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, cubeModel_2, view, projection, basicShapes.basicCube_2.overrideColor, basicShapes.basicCube_2.solidColor, 1.0f);
        //Rendering::DrawMesh(*cube_2.mesh); 
        Rendering::DrawMesh_FaceColors(shaderProgram, *basicShapes.basicCube_2.mesh, Demo::TestColors::cube_FaceColors); 
        
        
        
        // Cube version 2: placed behind the other cubes.  Testing textures 
        glm::mat4 cubeModel_v2 = AcornMath::BuildModelMatrix(
            basicShapes.cube_V2.transform.position,
            basicShapes.cube_V2.transform.scale,
            basicShapes.cube_V2.transform.rotationDegrees + basicShapes.cubeAuto.rotationDegrees
        );
        
        glActiveTexture(GL_TEXTURE0);                       // Select unit 0.
        glBindTexture(GL_TEXTURE_2D, testTexture_crate);     // Put this texture there.        
        
        Rendering::SetBasicShaderUniforms(
            shaderProgram, cubeModel_v2, view, projection, 
            basicShapes.cube_V2.overrideColor, basicShapes.cube_V2.solidColor, 1.0f, testTexture_crate != 0, 1.0f
        );
        shaderProgram.SetBool("uUseLighting", true);
        Rendering::DrawMesh(*basicShapes.cube_V2.mesh); 
        shaderProgram.SetBool("uUseLighting", false);
        
        
        
        // Sphere: placed on the right.
        glm::mat4 sphereModel_1 = AcornMath::BuildModelMatrix(
            basicShapes.sphere_1.transform.position + basicShapes.sphereAuto.translation,
            basicShapes.sphere_1.transform.scale,
            basicShapes.sphere_1.transform.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, sphereModel_1, view, projection, basicShapes.sphere_1.overrideColor, basicShapes.sphere_1.solidColor, 1.0f);
        Rendering::DrawMesh(*basicShapes.sphere_1.mesh);
        
        glm::mat4 sphereModel_2 = AcornMath::BuildModelMatrix(
            basicShapes.sphere_2.transform.position - basicShapes.sphereAuto.translation,
            basicShapes.sphere_2.transform.scale,
            basicShapes.sphere_2.transform.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, sphereModel_2, view, projection, basicShapes.sphere_2.overrideColor, basicShapes.sphere_2.solidColor, 1.0f);
        Rendering::DrawMesh(*basicShapes.sphere_2.mesh);
    }
    
    
    glm::vec3 SpinTheSun(float deltaTime, float& sunAngleDegrees)
    {
        constexpr float sunSpeedDegreesPerSecond = 65.0f;

        sunAngleDegrees += sunSpeedDegreesPerSecond * deltaTime;
        sunAngleDegrees = std::fmod(sunAngleDegrees, 360.0f);

        const glm::mat4 sunRotation = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(sunAngleDegrees),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        return glm::vec3(
            sunRotation * glm::vec4(-1.0f, -0.6f, -0.3f, 0.0f)
        );
    }
    
    
    glm::mat4 SpinTheCamera(float deltaTime, float& cameraAngleDegrees,
                        const glm::vec3& target)
    {
        constexpr float cameraSpeedDegreesPerSecond = 20.0f;

        cameraAngleDegrees += cameraSpeedDegreesPerSecond * deltaTime;
        cameraAngleDegrees = std::fmod(cameraAngleDegrees, 360.0f);

        const glm::mat4 orbitRotation = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(cameraAngleDegrees),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        const glm::vec3 startingOffset(4.0f, 5.0f, 7.0f);
        const glm::vec3 rotatedOffset =
            glm::mat3(orbitRotation) * startingOffset;

        const glm::vec3 cameraPosition = target + rotatedOffset;
        const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

        return glm::lookAt(cameraPosition, target, cameraUp);
    }
    
    
    void RenderBackpack(
        const ShaderProgram& shaderProgram, const glm::mat4& view, const glm::mat4& projection, 
        std::vector<OBJ_Loader_Assimp::ImportedPart>& importedParts, GLuint backpackTexture)
    {
        
        const glm::mat4 backpackModel(1.0f);
        glUniform1i(shaderProgram.GetBasicUniformLocations().textureLocation, 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, backpackTexture); 
        
        for (const auto& part : importedParts)
        {
            const bool useTexture = part.hasUVs && backpackTexture != 0;
            
            Rendering::SetBasicShaderUniforms(
                shaderProgram, backpackModel, view, projection,
                true, glm::vec3(0.75f), 1.0f, useTexture, 0.0f
            );
            
            shaderProgram.SetBool("uUseLighting", false);
            Rendering::DrawMesh(part.gpuMesh);
        }
    }
    
}






int main(int argc, char **argv) {
    
    using AcornData::Transform;
    using AcornData::Vertex;
    using AcornData::MeshData;
    using AcornData::GpuMesh;
    using AcornData::RenderObject; 
    
    
    std::cout << "hello world!" << '\n';   // std::endl;
    
    if (!InitializeProgram())
    {
        return -1;
    }
    
    GLFWwindow *window = CreateWindow();    
    if (window == nullptr)
    {
        glfwTerminate();
        return -1;
    }
    
    int frameBufferWidth;
    int frameBufferHeight;
    // set viewport once during setup, since registering the callback does not set it 
    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);
    
    
    ShaderProgram shaderProgram("shaders/basic.vert", "shaders/basic.frag");
    if (!shaderProgram.IsValid())
    {
        std::cerr << "[Startup] Required shader failed; exiting.\n";
        
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    CheckOpenGLErrors("shader setup");
    
    shaderProgram.CacheBasicUniformLocations();     // currently needs to be called every time after linking program 
    
    
    
    // loading the testing-texture
    stbi_set_flip_vertically_on_load(true);     // changes stb's loading setting for subsequent loads
    
    GLuint testTexture_crate = Texture::LoadTexture2D("textures/container.jpg"); 
    if (testTexture_crate == 0)
    {
        std::cerr << "[Warning] Failed to load texture: textures/container.jpg\n";
    }
    
    GLuint testTexture_face = Texture::LoadTexture2D("textures/awesomeface.png"); 
    if (testTexture_face == 0)
    {
        std::cerr << "[Warning] Failed to load texture: textures/awesomeface.png\n";
    }
    
    stbi_set_flip_vertically_on_load(false);
    
    
    
    // face culling
    GeometryRenderSettings renderSettings = {};
    renderSettings.bCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    
    // enable depth view 
    glEnable(GL_DEPTH_TEST);
    
    
    
    // ---- Runtime values ---- 
    double previousTime = glfwGetTime();
    
    renderSettings.bPerspectiveView = true; 
    
    
    // camera, perspective 
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    glm::vec3 cameraPosition = glm::vec3(4.0f, 5.0f, 7.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
    
    
    
    
    const GLint lightDirectionLocation = glGetUniformLocation(shaderProgram.ID, "ulightDirection");
    
    
    float sunAngleDegrees = 0.0f;
    float cameraAngleDegrees = 0.0f;
    
    
    
    // preparing rendering objects, week2 shapes  
    Demo::TestingDefaultShapes basicShapes {}; 
    
    
    
    // extra check because file had problems loading earlier
    const char* backpackPath = "models/backpack/backpack.obj";

    std::cout << "Working directory: "
              << std::filesystem::current_path() << '\n';
    std::cout << "Looking for: "
              << std::filesystem::absolute(backpackPath) << '\n';
    std::cout << "File exists: "
              << std::boolalpha << std::filesystem::exists(backpackPath) << '\n';
    
    // loading model using assimp 
    std::vector<OBJ_Loader_Assimp::ImportedPart> importedParts;
    
    for (const auto& mesh : OBJ_Loader_Assimp::LoadModel("models/backpack/backpack.obj"))
    {
        std::cout << "Backpack part: " << mesh.vertices.size()
                  << " vertices, " << mesh.indices.size() / 3
                  << " triangles\n";

        importedParts.push_back({
            Rendering::CreateGpuMesh(mesh),
            mesh.hasNormals,
            mesh.hasUVs
        });
    }
    
    // backpack texture
    stbi_set_flip_vertically_on_load(true);
    GLuint backpackTexture = Texture::LoadTexture2D("models/backpack/diffuse.jpg");
    stbi_set_flip_vertically_on_load(false);
    
    
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        processInput_ESCAPE(window);
        
        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);   // window background color, green-ish
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);   // window background color, black
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // get time and deltaTime 
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;
        
        // update runtime logic
        processInput_RenderSettings(window, renderSettings);
        
        
        
        glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);  
        // zero check skips drawing if there is no drawable area
        if (frameBufferWidth == 0 || frameBufferHeight == 0)
        {
            continue;
        }
        
        float aspectRatio = 
            static_cast<float>(frameBufferWidth) / 
            static_cast<float>(frameBufferHeight);
        
        
        if (renderSettings.bPerspectiveView)
        {
            projection = glm::perspective(glm::radians(70.0f), aspectRatio, nearPlane, farPlane);
        }
        else
        {
            projection = glm::ortho(-5.0f * aspectRatio, 5.0f * aspectRatio, -5.0f, 5.0f, nearPlane, farPlane);
        }
        
        
        
        
        // activate program
        shaderProgram.use(); 
        
        
        view = Demo::SpinTheCamera(deltaTime, cameraAngleDegrees, cameraTarget);
        
        
        //const glm::vec3 spinningSunDirection = Demo::SpinTheSun(deltaTime, sunAngleDegrees);
        //glUniform3fv(lightDirectionLocation, 1, glm::value_ptr(spinningSunDirection));
        
        
        
        //Demo::Update_TestingDefaultShapes(window, shaderProgram, basicShapes, deltaTime, view, projection, testTexture_crate);
        
        
        Demo::RenderBackpack(shaderProgram, view, projection, importedParts, backpackTexture);
        
        
        glfwSwapBuffers(window);
    }
    
    
    // temp backpack deletion
    for (auto& part : importedParts)
    {
        Rendering::DestroyGpuMesh(part.gpuMesh);
    }
    
    
    // Mesh cleanup
    Rendering::DestroyGpuMesh(basicShapes.pyramidGpu);
    Rendering::DestroyGpuMesh(basicShapes.cubeGpu);
    Rendering::DestroyGpuMesh(basicShapes.sphereGpu);
    
    Rendering::DestroyGpuMesh(basicShapes.pyramidGp_v2);
    Rendering::DestroyGpuMesh(basicShapes.cubeGpu_v2);
    
    
    glDeleteProgram(shaderProgram.ID);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



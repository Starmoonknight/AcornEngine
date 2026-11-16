// Project headers
#include "ShaderProgram.h" 


// Standard library
#include <array>        // Fixed-size list of OBJ face corners
#include <cassert>      // assert checks
#include <charconv>     // std::from_chars for OBJ indices
#include <cmath>        // Scalar math for geometry and animation
#include <cstddef>      // std::size_t and offsetof for vertex attributes
#include <filesystem>   // Asset paths and working-directory diagnostics
#include <fstream>      // std::ifstream to open OBJ files
#include <iostream>     // std::cout and std::cerr for diagnostics
#include <limits>       // std::numeric_limits for bounds checks
#include <sstream>      // std::istringstream to parse an OBJ line
#include <string>       // File lines and tokens
#include <string_view>  // Views of the fields in a face token
#include <system_error> // std::errc when checking parsed indices
#include <type_traits>  // std::is_standard_layout for Vertex
#include <utility>      // std::move when finishing a mesh
#include <vector>       // Mesh data and temporary OBJ attribute lists



// Third-party libraries - OpenGL
#include <glad/glad.h>                      // Keep glad include before GLFW include 
#include <glfw/glfw3.h>                     // Window, context, input, and timing

// Third-party libraries - GLM
#include <glm/glm.hpp>                      // Vectors, matrices, dot and cross products
#include <glm/gtc/constants.hpp>            // glm::pi and related constants
#include <glm/gtc/matrix_transform.hpp>     // Model, view, and projection transforms
#include <glm/gtc/type_ptr.hpp>             // glm::value_ptr for OpenGL uniforms

// Third-party libraries - stb 
#include "stb_image.h"                      // Texture image loading and vertical flip

// Third-party libraries - assimp 
#include <assimp/Importer.hpp>              // Assimp::Importer and ReadFile()
#include <assimp/postprocess.h>             // Triangulation and UV flip flags
#include <assimp/scene.h>                   // aiScene, aiMesh, and aiFace



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
        
        bool hasNormals = false;
        bool hasUVs = false;
    };
    
    
    struct RenderObject
    {        
        const GpuMesh* mesh= nullptr;
        Transform transform{};
        
        bool overrideColor = false;
        glm::vec3 solidColor{1.0f};
        
        GLuint textureID = 0; // 0 means no texture selected for this object.
    };
    
    
    struct Model
    {
        std::vector<GpuMesh> meshes;
        Transform transform{};
        
        // for tin´tint the whole mesh
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
        gpu.hasNormals = mesh.hasNormals;
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
        gpu.hasUVs = mesh.hasUVs;
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
    
    
    AcornData::Model CreateGpuModel(const std::vector<AcornData::MeshData>& meshes, GLenum usage = GL_STATIC_DRAW)
    {
        AcornData::Model model;
        model.meshes.reserve(meshes.size());

        for (const auto& mesh : meshes)
        {
            model.meshes.push_back(CreateGpuMesh(mesh, usage));
        }

        return model;
    }
    
    void DestroyGpuModel(AcornData::Model& model)
    {
        for (auto& mesh : model.meshes)
        {
            DestroyGpuMesh(mesh);
        }

        model.meshes.clear();
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
        shader.use();
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
        
        
        shader.use(); 
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
    
    
    
    void DrawModel(
        const ShaderProgram& shaderProgram, const glm::mat4& view, const glm::mat4& projection, 
        const AcornData::Model& model, const glm::mat4 modelMatrix, GLuint modelTexture)
    {
        shaderProgram.use();
        glUniform1i(shaderProgram.GetBasicUniformLocations().textureLocation, 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, modelTexture); 
        
        for (const auto& mesh : model.meshes)
        {
            const bool useTexture = mesh.hasUVs && modelTexture != 0;
            
            SetBasicShaderUniforms(
                shaderProgram, modelMatrix, view, projection,
                model.overrideColor, model.solidColor, 1.0f, useTexture, 0.0f
            );
            
            shaderProgram.SetBool("uUseLighting", mesh.hasNormals);
            DrawMesh(mesh);
        }
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
            
            
            // Record this as one face range, because quads are triangulated they are recorded as two faces. 
            mesh.faces.push_back({mesh.indices.size(), face.mNumIndices});
            
            
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
            {                
                mesh.indices.push_back(face.mIndices[j]);        
            }
        }
        
        return mesh; 
    }
    
    
    std::vector<AcornData::MeshData> LoadMesh(const char* filePath)
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
    
    
    // A wrapper for making a const std::vector<AcornData::MeshData> meshes = LoadMesh, 
    // and then calling Rendering::CreateGpuModel(meshes), with extra log info
    AcornData::Model UploadModel_Assimp(const char* filePath)
    {
        // extra check because file had problems loading earlier
        std::cout << "Working directory: "
                  << std::filesystem::current_path() << '\n';
        std::cout << "Looking for: "
                  << std::filesystem::absolute(filePath) << '\n';
        std::cout << "File exists: "
                  << std::boolalpha << std::filesystem::exists(filePath) << '\n';
    
        // reading all data and translating to the format my program use 
        const std::vector<AcornData::MeshData> meshes = LoadMesh(filePath);
        // extra info on what info was read: 
        for (const auto& mesh : meshes)
        {
            std::cout << "Model part: " << mesh.vertices.size()
                      << " vertices, " << mesh.indices.size() / 3
                      << " triangles\n";
        }
        
        // creating a model from the data 
        AcornData::Model model = Rendering::CreateGpuModel(meshes);
        return model;
    }
    
}



namespace OBJ_Loader
{
    
    struct FaceCorner
    {
        AcornData::Vertex vertex{};
        bool hasUV = false;
        bool hasNormal = false;
    };
    
    
    // Reads one index per call. 
    // Converts one OBJ face field, such as "12" or "-2", to a zero-based vector index. OBJ counts forward from 1, and C++ vectors count from 0.
    // On success, writes the result to index and returns true.
    // On failure, returns false and leaves index unchanged.
    bool ResolveIndex(std::string_view field, std::size_t count, std::size_t& index)
    {
        // Read one field from a face corner.
        if (field.empty())
        {
            return false;
        }

        long long objIndex = 0;
        
        // Parse the characters in this field. end points just past the last character.
        const char* end = field.data() + field.size();
        const auto result = std::from_chars(field.data(), end, objIndex);
        
        // Reject parse errors, leftover characters, and OBJ index 0.
        if (result.ec != std::errc{} || result.ptr != end || objIndex == 0)
        {
            return false;
        }
        
        // Keep the same count in a signed type so the calculation can go below zero.
        const auto signedCount = static_cast<long long>(count);
        
        // Positive OBJ indices count from 1. Negative indices count back from the end.
        // With four entries, 4 - 1 and 4 + (-1) both give index 3.
        const long long resolved = 
            objIndex > 0 ? objIndex - 1 : signedCount + objIndex;
        
        // Valid vector indices run from 0 through count - 1.
        if (resolved < 0 || resolved >= signedCount)
        {
            return false;
        }
        
        // Write the checked index back to the caller through the reference.
        index = static_cast<std::size_t>(resolved);
        return true;
    }
    
    
    
    // Reads one OBJ face corner, such as "3", "3/7", "3//2", or "3/7/2".
    // Looks up the position and any supplied UV and normal, then copies them into corner.
    // Pass a freshly initialized corner. On failure, returns false and corner may be partially filled.
    bool ReadFaceCorner(
        std::string_view token,                         // token: v/uv/nv - full info,  v// - only pos data,  v//nv - no uv data
        const std::vector<glm::vec3>& positions,        // list filled with all recorded vertex positions 
        const std::vector<glm::vec2>& uvs,              // list filled with all recorded uv coords
        const std::vector<glm::vec3>& normals,          // list filled with all recorded vertex normals 
        FaceCorner& corner)                             // result 
    {
        
        // start reading at the beginning of the current token: 
        // The position field ends at the first slash, or at the end of the token.
        const auto firstSlash = token.find('/');
        const auto positionField = token.substr(0, firstSlash);
        std::size_t index = 0;
        
        // find the position that matches this token's index from the saved positions   
        if (!ResolveIndex(positionField, positions.size(), index))
        {
            return false;                               // if resolving index fails, return false 
        }
        corner.vertex.position = positions[index];      // copy the coordinates at this index.
        
                                                        // npos is the special value find() returns when it can’t find what it's looking for,
        if (firstSlash == std::string_view::npos)       // if there was no slash anywhere in this token, means position-only corner 
        {
            return true; // v                           // Valid position-only corner, such as "3". 
        }
        
        
        // read the uv's between the slashes:
        // Make a new view starting after the first slash. "3/7/2" becomes "7/2".
        // The next find() and substr() count from the start of that view.
        const auto attributes = token.substr(firstSlash + 1);
        const auto secondSlash = attributes.find('/');
        const auto uvField = attributes.substr(0, secondSlash);
        
        if (!uvField.empty())                           // An empty UV field is allowed when the token supplies a normal, as in "3//2".
        {
            if (!ResolveIndex(uvField, uvs.size(), index))
            {
                return false;                           // so only return false if there was data to read that failed to resolve
            }
            corner.vertex.uv = uvs[index];              // copy the uv's at this index.
            corner.hasUV = true;                        // set the flag that will tell my program this mesh has valid uv's 
        }
        
        if (secondSlash == std::string_view::npos)      // if there was no second slash, but !uvField.empty(), then the corner should have recorded uv coords at this point 
        {
            return corner.hasUV; // v/vt                //when only one slash, it requires a UV, as in "3/7".
        }

        
        // read the normals after the two slashes:
        // A second slash introduces a required normal index, with or without a UV.
        const auto normalField = attributes.substr(secondSlash + 1);
        if (!ResolveIndex(normalField, normals.size(), index))
        {
            return false;
        }
        corner.vertex.normal = normals[index];          // copy the normals at this index.
        corner.hasNormal = true;                        // set the flag that will tell my program this mesh has valid normals 
        
        return true;                                    // corner has succeeded parsing and recording all information properly. 
    }
    
    
    // Reads the corners after `f` on one OBJ face line.
    // Appends the face's vertices and one or two triangles to mesh.
    // If the face is invalid or unsupported, returns false before modifying mesh.
    bool AppendFace(
        std::istringstream& fields,
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec2>& uvs,
        const std::vector<glm::vec3>& normals,
        AcornData::MeshData& mesh)
    {
        // Store up to four corners so the whole face can be checked before appending it.
        std::array<FaceCorner, 4> corners{};            // corners has four slots because the biggest face this loader accepts is a quad.
        std::size_t count = 0;                          // tracks how many slots have been filled 
        std::string token;                              // the next word read from the line.
        
        
        // Read one whitespace-separated corner token at a time, such as "3/7/2".
        while (fields >> token)                         // (fields >> token) skips whitespace, reads characters up to the next whitespace, and stores them in token
        {
            if (count == corners.size())                // a fifth corner exceeds this loader's triangle and quad support.
            {
                // N-gon function-call handling here later
                
                std::cerr << "N-gon found in loading model" << std::endl;
                return false; 
            }
            
            // read each token, filling into the temporary corners array
            if (!ReadFaceCorner(token, positions, uvs, normals, corners[count]))
            {
                return false;
            }
            ++count;                                    // tracks the actual corner amount for this face 
        }
        
        if (count < 3)                                  // needs to be a valid triangle face at least
        {
            return false;
        }
        
        // Quad face Handling:
        bool useFirstDiagonal = true;                   // quads need to be split into tris, try the a-c diagonal first. 
        if (count == 4)
        {
            const auto& a = corners[0].vertex.position;
            const auto& b = corners[1].vertex.position;
            const auto& c = corners[2].vertex.position;
            const auto& d = corners[3].vertex.position;
            
            // For each split, use the corner positions to compare the two triangle normals.
            //  On a flat quad, a positive dot product means the triangles face the same way.
            //  A negative result means they face opposite ways.
            //  Zero means at least one triangle has collapsed to a line or point.
            
            // Try the a-c diagonal with triangles (a, b, c) and (a, c, d).
            useFirstDiagonal = glm::dot(                
                glm::cross(b - a, c - a),           
                glm::cross(c - a, d - a)            
            ) > 0.0f;                                   
            
            // If a-c fails, try b-d with triangles (a, b, d) and (b, c, d).
            // Reject the face if this split also fails.
            if (!useFirstDiagonal && glm::dot(
                    glm::cross(b - a, d - a),       
                    glm::cross(c - b, d - b)        
                ) <= 0.0f)
            {
                return false;
            }
        }
        
        
        // Start with both flags set for a new mesh. 
        // Each stays true only if every corner appended to the mesh supplies that attribute.
        if (mesh.vertices.empty())                      
        {
            mesh.hasUVs = true;
            mesh.hasNormals = true;
        }
        
        // The current size is the index the first new vertex will get.
        const auto first = static_cast<unsigned int>(mesh.vertices.size()); // current size before anything is appended.  
        
        for (std::size_t i = 0; i < count; ++i)
        {
            mesh.vertices.push_back(corners[i].vertex);                     // copies each temp-corner’s assembled vertex into mesh.vertices
            mesh.hasUVs = mesh.hasUVs && corners[i].hasUV;                  // turns false when the first record lacking uv is added 
            mesh.hasNormals = mesh.hasNormals && corners[i].hasNormal;      // turns false when the first record lacking normals is added 
        }
        
        
        const std::size_t firstIndex = mesh.indices.size();                 // for adding FaceRange
        
        
        // Recording the indices in correct order: 
        // Each group of three indices draws one triangle from the new vertices.
        if (count == 3)
        {
            // For tris: appends the listed numbers to the end of the index vector.
            mesh.indices.insert(mesh.indices.end(), {first, first + 1, first + 2}); 
        }
        else if (useFirstDiagonal)      // For quads: appending order depends on what direction each tri should be drawn in
        {
            // Split along a-c into (a, b, c) and (a, c, d).
            mesh.indices.insert(mesh.indices.end(), {
                first, first + 1, first + 2,                        // (a, b, c)	Top-left, top-right, bottom-right 
                first, first + 2, first + 3                         // (a, c, d)	Top-left, bottom-right, bottom-left
            });
        }
        else
        {
            // Split along b-d into (a, b, d) and (b, c, d).
            mesh.indices.insert(mesh.indices.end(), {
                first, first + 1, first + 3,                        // (a, b, d)	Top-left, top-right, bottom-left
                first + 1, first + 2, first + 3                     // (b, c, d)	Top-right, bottom-right, bottom-left
            });
        }
        
        
        // Records one range per original f line to FaceRange
        mesh.faces.push_back({firstIndex, mesh.indices.size() - firstIndex});  
        
        
        return true;                                               // face was successfully appended to the MeshData 
    }
    
    
    
    // Reads an OBJ file into CPU-side meshes, using `o` lines as mesh boundaries.
    // Optionally flips the vertical UV coordinate. Returns an empty vector on failure.
    std::vector<AcornData::MeshData> LoadMesh(const char* filePath, bool flipUVs = false)
    {
        std::ifstream file(filePath);                               // creates an input file stream and tries to open the file at that path.
        if (!file)                                                  // file:	The OBJ file on disk
        {
            std::cerr << "Could not open OBJ: " << filePath << '\n';
            return {};
        }
        
        // Temp containers to keep data while reading, before being properly stored in a MeshData
        // Keeps the OBJ's raw attribute lists for the whole file.
        // Faces in later objects can reference entries defined before an `o` line.
        std::vector<glm::vec3> temp_positions;
        std::vector<glm::vec2> temp_uvs;
        std::vector<glm::vec3> temp_normals;
        
        std::vector<AcornData::MeshData> meshes;                    // the finished meshes, what LoadModel() eventually returns.
        AcornData::MeshData mesh;                                   // the current mesh being worked on that AppendFace() is building.
        
        
        // Lambda: If mesh that was worked on has recorded info, adds the current mesh to the finished list.
        const auto finishMesh = [&]()
        {
            if (!mesh.indices.empty())
            {
                meshes.push_back(std::move(mesh));                  // transfer the mesh's vectors without copying them.
                mesh = AcornData::MeshData{};                       // start a fresh mesh for the next object.
            }
        };
        
        
        std::string line;                                           // will hold the current line’s text.
        std::size_t lineNumber = 0;                                 // starts at 0 and increases after each successful line read, so the first line is numbered 1
        
        // Lambda:  Report the file, current line number, and reason for the failure.
        //          `return fail(...)` exits LoadMesh() with an empty result.
        const auto fail = [&](const char* reason)
        {
            std::cerr << filePath << ':' << lineNumber << ": " << reason << '\n';
            return std::vector<AcornData::MeshData>{};              // returns an empty vector to whoever called LoadModel()
        };
        
        // Read one line from the file per iteration, including blank lines.
        while (std::getline(file, line))                      // tries to read the next line from the OBJ file. 
        {                                                           // It replaces the contents of line, removes the newline delimiter from the text it stores,
            ++lineNumber;                                           // and leaves the stream ready to read the following line.
                                                                    // getline() returns the stream, and the while condition checks whether the read succeeded.
            
            // Lambda:  Strip comments by searching for the first #. 
            //          If one exists, erase(comment) deletes it and everything after it. 
            //          A line like "v 1 0 0 # right corner" becomes "v 1 0 0 ". A line that starts with # becomes empty.
            const auto comment = line.find('#');
            if (comment != std::string::npos)
            {
                line.erase(comment);
            }
            
            // Read the record type. The stream then holds the remaining fields.
            std::istringstream fields(line);                        // fields:	The text of the current line
            std::string record; 
            if (!(fields >> record))                                // (fields >> record) reads the first whitespace-separated word. For "v 1 0 0", record becomes "v" and fields is positioned just after that word, ready to read 1.
            {
                continue;                                           // if line was empty, whitespace-only, or just a comment that you erased, the read fails and skips line.
            }
            
            // Read record and store fields-content in correct temp container pos/uv/norm, 
            // or time to append faces, or finnish and store the current mesh.
            if (record == "v" || record == "vn")                    // expects three numbers to follow either of these signs
            {
                // Both records use three components. The record type chooses the list.
                glm::vec3 value{};
                if (!(fields >> value.x >> value.y >> value.z))
                {
                    return fail("Expected three position or normal components");
                }

                auto& values = record == "v" ? temp_positions : temp_normals;
                values.push_back(value);
            }
            else if (record == "vt")                                // expects two numbers to follow this sign
            {
                // U is required. V starts at zero and can be omitted.
                glm::vec2 uv{0.0f};
                if (!(fields >> uv.x))
                {
                    return fail("Expected a texture coordinate");
                }
                
                // Skip trailing spaces before checking whether a V field is present.
                fields >> std::ws;
                if (!fields.eof() && !(fields >> uv.y))
                {
                    return fail("Invalid second texture coordinate");
                }

                if (flipUVs)
                {
                    uv.y = 1.0f - uv.y;
                }
                temp_uvs.push_back(uv);
            }
            else if (record == "o")                                 // reached a new mesh, so finnish up the current one 
            {
                // A new object ends the current mesh. This loader discards the name.
                finishMesh();
            }
            else if (record == "f")                                 // start making faces of the collected v/vt/vn records 
            {
                // Resolve the corner references and append vertices and triangle indices.
                if (!AppendFace(fields, temp_positions, temp_uvs, temp_normals, mesh))
                {
                    return fail("Invalid face or index. Expected a triangle or quad.");
                }
            }
        }
        
        // Check for a serious stream error after the read loop ends.
        if (file.bad())                                             
        {
            return fail("Error while reading OBJ");
        }
        
        
        // The last mesh ends at EOF, so save it here, and then return the result.
        finishMesh();                                               

        if (meshes.empty())
        {
            return fail("No faces found");
        }

        return meshes;
    }
    
    
    // Loads an OBJ with the custom parser, logs each mesh's size, and creates a GPU model.
    // Call this after initializing OpenGL with a current context.
    AcornData::Model UploadModel(const char* filePath, bool flipUVs = false)
    {
        // Show where the program resolves the path when diagnosing missing assets.
        std::cout << "Working directory: "
                  << std::filesystem::current_path() << '\n';
        std::cout << "Looking for: "
                  << std::filesystem::absolute(filePath) << '\n';
        std::cout << "File exists: "
                  << std::boolalpha << std::filesystem::exists(filePath) << '\n';
        
        // reading all data and translating to the format my program use 
        const std::vector<AcornData::MeshData> meshes = LoadMesh(filePath, flipUVs);
        // extra info on what info was read: 
        for (const auto& mesh : meshes)
        {
            std::cout << "Model part: " << mesh.vertices.size()
                      << " vertices, " << mesh.indices.size() / 3
                      << " triangles\n";
        }
        
        // creating a model from the data 
        AcornData::Model model = Rendering::CreateGpuModel(meshes);
        return model;
    }
    
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
            
            values.bScaleX = false;
            values.bScaleY = false;
            values.bScaleZ = false;
            
            values.bRotateX = false;
            values.bRotateY = false;
            values.bRotateZ = false;
            
            values.bTranslateX = false;
            values.bTranslateY = false;
            values.bTranslateZ = false;
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
    
    
    struct Testing_DefaultShapes 
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
        
        Testing_DefaultShapes()
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
        GLFWwindow* window, const ShaderProgram& shaderProgram, Testing_DefaultShapes& basicShapes,
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
    
    
    
    
    struct Testing_TwoBackpacks
    {
        using Model = AcornData::Model;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        // loading model using assimp 
        const char* backpackPath = "models/backpack/backpack.obj";
        const char* backpackTexturePath = "models/backpack/diffuse.jpg";
        
        
        // Left Backpack, using assimp loader
        Model backpack_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(backpackPath); 
        
        // right Backpack, using my loader
        Model backpack_mine = OBJ_Loader::UploadModel(backpackPath, true);
        
        
        Testing_TwoBackpacks()
        {
            stbi_set_flip_vertically_on_load(true);
            GLuint textureID = Texture::LoadTexture2D(backpackTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            backpack_assimp.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
            backpack_assimp.overrideColor = true;
            backpack_assimp.solidColor = glm::vec3(0.75f);
            backpack_assimp.textureID = textureID;
            
            
            backpack_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            backpack_mine.overrideColor = true;
            backpack_mine.solidColor = glm::vec3(0.75f);
            backpack_mine.textureID = textureID;
        }
    };
    
    // Left Backpack = assimp obj loader, Right Backpack = my obj loader
    void Update_DrawTwoBackpacks(GLFWwindow* window, const ShaderProgram& shaderProgram, Testing_TwoBackpacks& models,
        float deltaTime, const glm::mat4& view, const glm::mat4& projection)
    {
        HandleRuntimeInputs(window, models.runtime_Inputs, models.runtime_Info);
        UpdateTransforms(models.runtime_Info, deltaTime);
        
        
        const glm::mat4 backpackMatrix_Left = AcornMath::BuildModelMatrix(
            models.backpack_assimp.transform.position + models.runtime_Info.translation,
            models.backpack_assimp.transform.scale * models.runtime_Info.scale,
            models.backpack_assimp.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.backpack_assimp, backpackMatrix_Left, models.backpack_assimp.textureID);
        
        
        const glm::mat4 backpackMatrix_Right = AcornMath::BuildModelMatrix(
            models.backpack_mine.transform.position + models.runtime_Info.translation,
            models.backpack_mine.transform.scale * models.runtime_Info.scale,
            models.backpack_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.backpack_mine, backpackMatrix_Right, models.backpack_mine.textureID);
    }
    
    
    
    struct Testing_TwoTeapots
    {
        using Model = AcornData::Model;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        // loading model using assimp 
        const char* teapotPath = "models/teapot/teapot.obj";
        const char* teapotTexturePath = "models/teapot/default.png";
        
        // Left Backpack, using assimp loader
        Model teapot_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(teapotPath); 
        
        // right Backpack, using my loader
        Model teapot_mine = OBJ_Loader::UploadModel(teapotPath, true);
        
        
        Testing_TwoTeapots()
        {
            stbi_set_flip_vertically_on_load(true);
            GLuint textureID = Texture::LoadTexture2D(teapotTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            teapot_assimp.transform.position = glm::vec3(-3.0f, 0.0f, 0.0f);
            teapot_assimp.transform.scale = glm::vec3(0.025f);
            teapot_assimp.overrideColor = true;
            teapot_assimp.solidColor = glm::vec3(0.75f);
            teapot_assimp.textureID = textureID;
            
            
            teapot_mine.transform.position = glm::vec3(3.0f, 0.0f, 0.0f);
            teapot_mine.transform.scale = glm::vec3(0.025f);
            teapot_mine.overrideColor = true;
            teapot_mine.solidColor = glm::vec3(0.75f);
            teapot_mine.textureID = textureID;
        }
    };
    
    // Left Backpack = assimp obj loader, Right Backpack = my obj loader
    void Update_DrawTwoTeapots(GLFWwindow* window, const ShaderProgram& shaderProgram, Testing_TwoTeapots& models,
        float deltaTime, const glm::mat4& view, const glm::mat4& projection)
    {
        HandleRuntimeInputs(window, models.runtime_Inputs, models.runtime_Info);
        UpdateTransforms(models.runtime_Info, deltaTime);
        
        
        const glm::mat4 teapotMatrix_Left = AcornMath::BuildModelMatrix(
            models.teapot_assimp.transform.position + models.runtime_Info.translation,
            models.teapot_assimp.transform.scale * models.runtime_Info.scale,
            models.teapot_assimp.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.teapot_assimp, teapotMatrix_Left, models.teapot_assimp.textureID);
        
        
        const glm::mat4 teapotMatrix_Right = AcornMath::BuildModelMatrix(
            models.teapot_mine.transform.position + models.runtime_Info.translation,
            models.teapot_mine.transform.scale * models.runtime_Info.scale,
            models.teapot_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.teapot_mine, teapotMatrix_Right, models.teapot_mine.textureID);
    }
    
    
    
    
    struct Testing_TwoIpshead
    {
        using Model = AcornData::Model;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        // loading model using assimp 
        const char* lpsheadPath = "models/lpshead/head.obj";               // might be "models/lpshead/head.OBJ"
        const char* lpsheadTexturePath = "models/lpshead/rendered.jpg";
        //const char* lpsheadTexturePath = "models/lpshead/lambertian.jpg";
        
        
        // Left Backpack, using assimp loader
        Model lpshead_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(lpsheadPath); 
        
        // right Backpack, using my loader
        Model lpshead_mine = OBJ_Loader::UploadModel(lpsheadPath, true);
        
        
        Testing_TwoIpshead()
        {
            stbi_set_flip_vertically_on_load(true);
            GLuint textureID = Texture::LoadTexture2D(lpsheadTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            lpshead_assimp.transform.position = glm::vec3(-2.0f, 2.0f, 0.0f);
            lpshead_assimp.transform.scale = glm::vec3(10.0f);
            lpshead_assimp.overrideColor = true;
            lpshead_assimp.solidColor = glm::vec3(0.75f);
            lpshead_assimp.textureID = textureID;
            
            
            lpshead_mine.transform.position = glm::vec3(2.0f, 2.0f, 0.0f);
            lpshead_mine.transform.scale = glm::vec3(10.0f);
            lpshead_mine.overrideColor = true;
            lpshead_mine.solidColor = glm::vec3(0.75f);
            lpshead_mine.textureID = textureID;
        }
    };
    
    // Left Backpack = assimp obj loader, Right Backpack = my obj loader
    void Update_DrawTwoIpshead(GLFWwindow* window, const ShaderProgram& shaderProgram, Testing_TwoIpshead& models,
        float deltaTime, const glm::mat4& view, const glm::mat4& projection)
    {
        HandleRuntimeInputs(window, models.runtime_Inputs, models.runtime_Info);
        UpdateTransforms(models.runtime_Info, deltaTime);
        
        
        const glm::mat4 lpsheadMatrix_Left = AcornMath::BuildModelMatrix(
            models.lpshead_assimp.transform.position + models.runtime_Info.translation,
            models.lpshead_assimp.transform.scale * models.runtime_Info.scale,
            models.lpshead_assimp.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.lpshead_assimp, lpsheadMatrix_Left, models.lpshead_assimp.textureID);
        
        
        const glm::mat4 lpsheadMatrix_Right = AcornMath::BuildModelMatrix(
            models.lpshead_mine.transform.position + models.runtime_Info.translation,
            models.lpshead_mine.transform.scale * models.runtime_Info.scale,
            models.lpshead_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.lpshead_mine, lpsheadMatrix_Right, models.lpshead_mine.textureID);
    }
    
    
    
    
    struct Testing_TwoCubes
    {
        using Model = AcornData::Model;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        // loading model using assimp 
        const char* cubePath = "models/cube/cube.obj";
        const char* cubeTexturePath = "models/cube/default.png";
        
        
        // Left Backpack, using assimp loader
        Model cube_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(cubePath); 
        
        // right Backpack, using my loader
        Model cube_mine = OBJ_Loader::UploadModel(cubePath, true);
        
        
        Testing_TwoCubes()
        {
            stbi_set_flip_vertically_on_load(true);
            GLuint textureID = Texture::LoadTexture2D(cubeTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            cube_assimp.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
            cube_assimp.transform.scale = glm::vec3(2.0f);
            cube_assimp.overrideColor = true;
            cube_assimp.solidColor = glm::vec3(0.75f);
            cube_assimp.textureID = textureID;
            
            
            cube_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            cube_mine.transform.scale = glm::vec3(2.0f);
            cube_mine.overrideColor = true;
            cube_mine.solidColor = glm::vec3(0.75f);
            cube_mine.textureID = textureID;
        }
    };
    
    // Left Backpack = assimp obj loader, Right Backpack = my obj loader
    void Update_DrawTwoCubes(GLFWwindow* window, const ShaderProgram& shaderProgram, Testing_TwoCubes& models,
        float deltaTime, const glm::mat4& view, const glm::mat4& projection)
    {
        HandleRuntimeInputs(window, models.runtime_Inputs, models.runtime_Info);
        UpdateTransforms(models.runtime_Info, deltaTime);
        
        
        const glm::mat4 cubeMatrix_Left = AcornMath::BuildModelMatrix(
            models.cube_assimp.transform.position + models.runtime_Info.translation,
            models.cube_assimp.transform.scale * models.runtime_Info.scale,
            models.cube_assimp.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.cube_assimp, cubeMatrix_Left, models.cube_assimp.textureID);
        
        
        const glm::mat4 cubeMatrix_Right = AcornMath::BuildModelMatrix(
            models.cube_mine.transform.position + models.runtime_Info.translation,
            models.cube_mine.transform.scale * models.runtime_Info.scale,
            models.cube_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.cube_mine, cubeMatrix_Right, models.cube_mine.textureID);
    }
    
    
    
    
    struct Testing_TwoPines
    {
        using Model = AcornData::Model;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        // loading model using assimp 
        const char* pinePath = "models/pine/scrubPine.obj";
        const char* pineTexturePath = "models/pine/LushPine.png";
        
        
        // Left Backpack, using assimp loader
        Model pine_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(pinePath); 
        
        // right Backpack, using my loader
        Model pine_mine = OBJ_Loader::UploadModel(pinePath, true);
        
        
        Testing_TwoPines()
        {
            stbi_set_flip_vertically_on_load(true);
            GLuint textureID = Texture::LoadTexture2D(pineTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            pine_assimp.transform.position = glm::vec3(-3.0f, -2.0f, 0.5f);
            pine_assimp.transform.scale = glm::vec3(0.01f);
            pine_assimp.overrideColor = true;
            pine_assimp.solidColor = glm::vec3(0.75f);
            pine_assimp.textureID = textureID;
            
            
            pine_mine.transform.position = glm::vec3(3.0f, -2.0f, 0.5f);
            pine_mine.transform.scale = glm::vec3(0.01f);
            pine_mine.overrideColor = true;
            pine_mine.solidColor = glm::vec3(0.75f);
            pine_mine.textureID = textureID;
        }
    };
    
    // Left Backpack = assimp obj loader, Right Backpack = my obj loader
    void Update_DrawTwoPines(GLFWwindow* window, const ShaderProgram& shaderProgram, Testing_TwoPines& models,
        float deltaTime, const glm::mat4& view, const glm::mat4& projection)
    {
        HandleRuntimeInputs(window, models.runtime_Inputs, models.runtime_Info);
        UpdateTransforms(models.runtime_Info, deltaTime);
        
        
        const glm::mat4 pineMatrix_Left = AcornMath::BuildModelMatrix(
            models.pine_assimp.transform.position + models.runtime_Info.translation,
            models.pine_assimp.transform.scale * models.runtime_Info.scale,
            models.pine_assimp.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.pine_assimp, pineMatrix_Left, models.pine_assimp.textureID);
        
        
        const glm::mat4 pineMatrix_Right = AcornMath::BuildModelMatrix(
            models.pine_mine.transform.position + models.runtime_Info.translation,
            models.pine_mine.transform.scale * models.runtime_Info.scale,
            models.pine_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.pine_mine, pineMatrix_Right, models.pine_mine.textureID);
    }
    
    
    
    
    struct Testing_TwoSportCars
    {
        using Model = AcornData::Model;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        // loading model using assimp 
        const char* sportCarsPath = "models/sportsCar/sportsCar.obj";
        //const char* sportCarTexturePath = "models/sportsCar/";        // no texture
        
        
        // Left Backpack, using assimp loader
        Model sportCar_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(sportCarsPath); 
        
        // right Backpack, using my loader
        Model sportCar_mine = OBJ_Loader::UploadModel(sportCarsPath, true);
        
        
        Testing_TwoSportCars()
        {
            //stbi_set_flip_vertically_on_load(true);
            //GLuint textureID = Texture::LoadTexture2D(sportCarTexturePath);
            //stbi_set_flip_vertically_on_load(false);
            
            sportCar_assimp.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
            sportCar_assimp.overrideColor = true;
            sportCar_assimp.solidColor = glm::vec3(0.75f);
            //sportCar_assimp.textureID = textureID;
            
            
            sportCar_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            sportCar_mine.overrideColor = true;
            sportCar_mine.solidColor = glm::vec3(0.75f);
            //sportCar_mine.textureID = textureID;
        }
    };
    
    // Left Backpack = assimp obj loader, Right Backpack = my obj loader
    void Update_DrawTwoSportCars(GLFWwindow* window, const ShaderProgram& shaderProgram, Testing_TwoSportCars& models,
        float deltaTime, const glm::mat4& view, const glm::mat4& projection)
    {
        HandleRuntimeInputs(window, models.runtime_Inputs, models.runtime_Info);
        UpdateTransforms(models.runtime_Info, deltaTime);
        
        
        const glm::mat4 sportCarMatrix_Left = AcornMath::BuildModelMatrix(
            models.sportCar_assimp.transform.position + models.runtime_Info.translation,
            models.sportCar_assimp.transform.scale * models.runtime_Info.scale,
            models.sportCar_assimp.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.sportCar_assimp, sportCarMatrix_Left, models.sportCar_assimp.textureID);
        
        
        const glm::mat4 sportCarMatrix_Right = AcornMath::BuildModelMatrix(
            models.sportCar_mine.transform.position + models.runtime_Info.translation,
            models.sportCar_mine.transform.scale * models.runtime_Info.scale,
            models.sportCar_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.sportCar_mine, sportCarMatrix_Right, models.sportCar_mine.textureID);
    }
    
    
    
    enum class TestScene
    {
        DefaultShapes = 1,
        Backpacks = 2,
        Teapots = 3,
        Cubes = 4,
        Pines = 5,
        SportCars = 6,
        Ipshead = 7,
        
        AllModels = 8
    };
    
    
}






int main(int argc, char **argv) {
    
    using AcornData::Transform;
    using AcornData::Vertex;
    using AcornData::MeshData;
    using AcornData::GpuMesh;
    using AcornData::RenderObject; 
    using AcornData::Model; 
    
    
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
    Demo::Testing_DefaultShapes basicShapes {}; 
    
    
    //  preparing rendering objects, week3 models 
    Demo::Testing_TwoBackpacks twoBackpacks {};
    
    Demo::Testing_TwoTeapots twoTeapots {};
    Demo::Testing_TwoIpshead twoIpshead {};
    Demo::Testing_TwoCubes twoCubes {};
    Demo::Testing_TwoPines twoPines {};
    Demo::Testing_TwoSportCars twoSportCars {};
    
    int selectedTest = static_cast<int>(Demo::TestScene::Backpacks);
    
    bool useBlackBackground = true;
    bool previousZeroDown = false;
    
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        processInput_ESCAPE(window);
        
        // switch between bright and dark background with key press 0 
        const bool zeroDown = glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS;
        if (zeroDown && !previousZeroDown)
        {
            useBlackBackground = !useBlackBackground;
        }
        previousZeroDown = zeroDown;
        if (useBlackBackground)
        {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        }
        
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
        
        
        
        // - Backpack, teapot, and cube render with both loaders.
        // - Pine renders with both loaders. More of the tree is visible with back-face culling disabled, which suggests some surfaces need two-sided rendering.
        // - Sports car and head render only through Assimp. The custom loader aborts on quads that fail both diagonal checks.
        // - The head test uses `rendered.jpg`. Its material specifies `lambertian.jpg` as the diffuse texture, so the texture needs retesting with that image.
        
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) selectedTest = 1;
        else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) selectedTest = 2;
        else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) selectedTest = 3;
        else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) selectedTest = 4;
        else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) selectedTest = 5;
        else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) selectedTest = 6;
        else if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) selectedTest = 7;
        else if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) selectedTest = 8;

        switch (static_cast<Demo::TestScene>(selectedTest))
        {
        case Demo::TestScene::DefaultShapes:
            Demo::Update_TestingDefaultShapes(
                window, shaderProgram, basicShapes, deltaTime, view, projection,
                testTexture_crate);
            break;

        case Demo::TestScene::Backpacks:
            Demo::Update_DrawTwoBackpacks(
                window, shaderProgram, twoBackpacks, deltaTime, view, projection);
            break;

        case Demo::TestScene::Teapots:
            Demo::Update_DrawTwoTeapots(
                window, shaderProgram, twoTeapots, deltaTime, view, projection);
            break;

        case Demo::TestScene::Cubes:
            Demo::Update_DrawTwoCubes(
                window, shaderProgram, twoCubes, deltaTime, view, projection);
            break;

        case Demo::TestScene::Pines:
            Demo::Update_DrawTwoPines(
                window, shaderProgram, twoPines, deltaTime, view, projection);
            break;

        case Demo::TestScene::SportCars:
            Demo::Update_DrawTwoSportCars(
                window, shaderProgram, twoSportCars, deltaTime, view, projection);
            break;

        case Demo::TestScene::Ipshead:
            Demo::Update_DrawTwoIpshead(
                window, shaderProgram, twoIpshead, deltaTime, view, projection);
            break;
            
            case Demo::TestScene::AllModels:
            Demo::Update_DrawTwoBackpacks(window, shaderProgram, twoBackpacks, deltaTime, view, projection);
            
            Demo::Update_DrawTwoTeapots(window, shaderProgram, twoTeapots, deltaTime, view, projection);
            Demo::Update_DrawTwoCubes(window, shaderProgram, twoCubes, deltaTime, view, projection);
            
            Demo::Update_DrawTwoPines(window, shaderProgram, twoPines, deltaTime, view, projection);
            
            Demo::Update_DrawTwoSportCars(window, shaderProgram, twoSportCars, deltaTime, view, projection);
            Demo::Update_DrawTwoIpshead(window, shaderProgram, twoIpshead, deltaTime, view, projection);
            break;
        }
        
        
        
        
        
        glfwSwapBuffers(window);
    }
    
    
    
    // Mesh cleanup
    Rendering::DestroyGpuMesh(basicShapes.pyramidGpu);
    Rendering::DestroyGpuMesh(basicShapes.cubeGpu);
    Rendering::DestroyGpuMesh(basicShapes.sphereGpu);
    
    Rendering::DestroyGpuMesh(basicShapes.pyramidGp_v2);
    Rendering::DestroyGpuMesh(basicShapes.cubeGpu_v2);
    
    
    // Model cleanup
    //Rendering::DestroyGpuModel(backpack);    
    Rendering::DestroyGpuModel(twoBackpacks.backpack_assimp);    
    Rendering::DestroyGpuModel(twoBackpacks.backpack_mine);  
    
    Rendering::DestroyGpuModel(twoTeapots.teapot_assimp);    
    Rendering::DestroyGpuModel(twoTeapots.teapot_mine);    
    
    Rendering::DestroyGpuModel(twoIpshead.lpshead_assimp);    
    Rendering::DestroyGpuModel(twoIpshead.lpshead_mine);    
    
    Rendering::DestroyGpuModel(twoCubes.cube_assimp);    
    Rendering::DestroyGpuModel(twoCubes.cube_mine);    
    
    Rendering::DestroyGpuModel(twoPines.pine_assimp);    
    Rendering::DestroyGpuModel(twoPines.pine_mine);    
    
    Rendering::DestroyGpuModel(twoSportCars.sportCar_assimp);    
    Rendering::DestroyGpuModel(twoSportCars.sportCar_mine);    
    
    
    glDeleteProgram(shaderProgram.ID);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



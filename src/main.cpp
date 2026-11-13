// Project headers
#include "ShaderProgram.h" 

// Standard Library
#include <cassert>          // assert
#include <type_traits>      // std::is_standard_layout
#include <limits>           // std::numeric_limits
#include <cmath>
#include <vector>
#include <cstddef>
#include <iostream>

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


namespace
{
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    
    constexpr float nearPlane = 0.1f;
    constexpr float farPlane = 100.0f;
    
    
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
        std::vector<glm::vec3> colors;          // Empty when this mesh has no per-vertex colors.
        
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
    AcornData::MeshData MakeBasicPyramid()
    {
        using AcornData::MeshData;   
        
        MeshData mesh;
        
        mesh.vertices = {
            { glm::vec3( 0.5f, -0.5f, -0.5f) }, // 0, bottom, back right 
            { glm::vec3(-0.5f, -0.5f, -0.5f) }, // 1, bottom, back left 
            { glm::vec3(-0.5f, -0.5f,  0.5f) }, // 2, bottom, forward left 
            { glm::vec3( 0.5f, -0.5f,  0.5f) }, // 3, bottom, forward right
            { glm::vec3( 0.0f,  0.5f,  0.0f) }  // 4, up, middle
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
        
        return mesh;
    }
    
    AcornData::MeshData MakeBasicCube()
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
        
        return mesh; 
    }
    
    AcornData::MeshData MakeBasicCube_v2()
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
        
        mesh.faces = {
        {  0, 6 }, // Front.
        {  6, 6 }, // Back.
        { 12, 6 }, // Bottom.
        { 18, 6 }, // Top.
        { 24, 6 }, // Left.
        { 30, 6 }  // Right.
        };
        
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
        // Colours are absent, or there is one colour per vertex.
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
                2, 3, GL_FLOAT, GL_FALSE, 
                static_cast<GLsizei>(sizeof(Vertex)), 
                reinterpret_cast<const void*>(offsetof(Vertex, normal))
            );
            
            glEnableVertexAttribArray(2);
        }
        else
        {
            glDisableVertexAttribArray(2);
        }
        
        // uv,              in attribute slot 3
        if (mesh.hasUVs)
        {
            glVertexAttribPointer(
                3, 2, GL_FLOAT, GL_FALSE,
                static_cast<GLsizei>(sizeof(Vertex)),
                reinterpret_cast<const void*>(offsetof(Vertex, uv))
            );
            
            glEnableVertexAttribArray(3);
        }
        else
        {
            glDisableVertexAttribArray(3);
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
                1, 3, GL_FLOAT, GL_FALSE, 
                static_cast<GLsizei>(sizeof(glm::vec3)), 
                nullptr
            );
            glEnableVertexAttribArray(1);
        }
        else
        {
            glDisableVertexAttribArray(1);
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
    
    void SetBasicShaderUniforms(
        const ShaderProgram& shader,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection,
        bool colorOverride = false,
        glm::vec3 color = glm::vec3(1.0f))
    {
        glUseProgram(shader.ID); 
        const BasicUniformLocations& uniforms = shader.GetBasicUniformLocations();
        
        // updates uniforms on the currently active program using the saved uniform locations 
        glUniformMatrix4fv(uniforms.modelLocation, 1 ,GL_FALSE,  glm::value_ptr(model));
        glUniformMatrix4fv(uniforms.viewLocation, 1 ,GL_FALSE,  glm::value_ptr(view));
        glUniformMatrix4fv(uniforms.projectionLocation, 1 ,GL_FALSE,  glm::value_ptr(projection));
        
        glUniform1i(uniforms.overrideColorLocation, colorOverride);
        glUniform3fv(uniforms.colorLocation, 1, glm::value_ptr(color));  
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
        
        // using STBI_rgb_alpha instead of 0 would make the returned unsigned char *pixels layout consistently four channels
        unsigned char *pixels = stbi_load(filePath, &width, &height, &channelsInFile, 0);  
        
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(pixels);

        return textureID;
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
    
    // currently needs to be called every time after linking program 
    shaderProgram.CacheBasicUniformLocations();
    
    
    // preparing rendering objects  
    MeshData pyramidData = BasicMeshes::MakeBasicPyramid();
    MeshData cubeData = BasicMeshes::MakeBasicCube();
    MeshData cube2Data = BasicMeshes::MakeBasicCube_v2();
    MeshData sphereData = BasicMeshes::MakeBasicSphere();
    
    GpuMesh pyramidGpu = Rendering::CreateGpuMesh(pyramidData);
    GpuMesh cubeGpu = Rendering::CreateGpuMesh(cubeData);
    GpuMesh cubeGpu_v2 = Rendering::CreateGpuMesh(cube2Data);
    GpuMesh sphereGpu = Rendering::CreateGpuMesh(sphereData);
    
    
    // making object instances
    RenderObject pyramid_1{&pyramidGpu};
    pyramid_1.transform.position = glm::vec3(-4.0f, 0.0f, 1.0f);
    
    RenderObject pyramid_2{&pyramidGpu, Transform{}, true, Palette::blue};
    pyramid_2.transform.position = glm::vec3(-4.0f, 0.0f, -2.5f);
    pyramid_2.transform.rotationDegrees = glm::vec3(180.0f, 0.0f, 0.0f);
    
    RenderObject pyramid_manual{&pyramidGpu};
    pyramid_manual.transform.position = glm::vec3(0.0f, 0.0f, 4.0f);
    
    
    RenderObject cube_1{&cubeGpu};
    cube_1.transform.position = glm::vec3(0.0f, 0.0f, 1.0f);
    
    RenderObject cube_2{&cubeGpu, Transform{}, true, Palette::pink};
    cube_2.transform.position = glm::vec3(0.0f, 0.0f, -2.5f);
    
    RenderObject cube_V2{&cubeGpu_v2};
    cube_V2.transform.position = glm::vec3(0.0f, 0.0f, -7.0f);
    
    
    RenderObject sphere_1{&sphereGpu};
    sphere_1.transform.position = glm::vec3(3.0f, 0.0f, -1.0f);
    
    RenderObject sphere_2{&sphereGpu, Transform{}, true, Palette::forestGreen};
    sphere_2.transform.position = glm::vec3(4.5f, 0.0f, -2.5f);
    
    
    
    // camera, perspective 
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    glm::vec3 cameraPosition = glm::vec3(4.0f, 5.0f, 7.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
    
    
    
    // face culling
    GeometryRenderSettings renderSettings = {};
    renderSettings.bCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    
    // enable depth view 
    glEnable(GL_DEPTH_TEST);
    
    
    
    // runtime values
    double previousTime = glfwGetTime();
    
    renderSettings.bPerspectiveView = true; 
    
    // controlled by keyboard toggles.
    Demo::GeometryRuntimeValues_Inputs runtime_Inputs = {}; 
    Demo::GeometryRuntimeValues_Transforms runtime_Info = {};
    // Automatic animations.
    Demo::GeometryRuntimeValues_Transforms cubeAuto;
    cubeAuto.bRotateY = true;
    Demo::GeometryRuntimeValues_Transforms sphereAuto;
    sphereAuto.bTranslateZ = true;
    Demo::GeometryRuntimeValues_Transforms pyramidAuto;
    pyramidAuto.bScaleY = true;
    
    
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        processInput_ESCAPE(window);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);   // window background color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // get time and deltaTime 
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;
        
        // update runtime logic
        processInput_RenderSettings(window, renderSettings);
        
        Demo::HandleRuntimeInputs(window, runtime_Inputs, runtime_Info);

        Demo::UpdateTransforms(runtime_Info, deltaTime);
        Demo::UpdateTransforms(cubeAuto, deltaTime);
        Demo::UpdateTransforms(sphereAuto, deltaTime);
        Demo::UpdateTransforms(pyramidAuto, deltaTime);
        
        
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
        
        
        
        glm::vec3 manualPosition = runtime_Info.translation;
        glm::vec3 manualScale = runtime_Info.scale;
        glm::vec3 manualRotationDegrees = runtime_Info.rotationDegrees;
        
        
        // Pyramid: placed on the left.
        glm::mat4 pyramidModel_1 = AcornMath::BuildModelMatrix(
            pyramid_1.transform.position,
            pyramid_1.transform.scale * pyramidAuto.scale,
            pyramid_1.transform.rotationDegrees);
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidModel_1, view, projection, pyramid_1.overrideColor, pyramid_1.solidColor);        
        Rendering::DrawMesh(*pyramid_1.mesh);
        
        glm::mat4 pyramidModel_2 = AcornMath::BuildModelMatrix(
            pyramid_2.transform.position,
            pyramid_2.transform.scale * pyramidAuto.scale,
            pyramid_2.transform.rotationDegrees);        
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidModel_2, view, projection, pyramid_2.overrideColor, pyramid_2.solidColor);
        Rendering::DrawMesh(*pyramid_2.mesh);
        
        
        // manually controlled pyramid
        glm::mat4 pyramidModel_Manual = AcornMath::BuildModelMatrix(
            pyramid_manual.transform.position + manualPosition,
            pyramid_manual.transform.scale * manualScale,
            pyramid_manual.transform.rotationDegrees + manualRotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidModel_Manual, view, projection, pyramid_manual.overrideColor, pyramid_manual.solidColor);
        //Rendering::DrawMesh(*pyramid_manual.mesh);
        Rendering::DrawMesh_FaceColors(shaderProgram, *pyramid_manual.mesh, Demo::TestColors::pyramid_FaceColors); 
        
        
        // Cube version 1: placed in the middle.
        glm::mat4 cubeModel_1 = AcornMath::BuildModelMatrix(
            cube_1.transform.position,
            cube_1.transform.scale,
            cube_1.transform.rotationDegrees + cubeAuto.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, cubeModel_1, view, projection, cube_1.overrideColor, cube_1.solidColor);
        Rendering::DrawMesh(*cube_1.mesh);
        
        
        glm::mat4 cubeModel_2 = AcornMath::BuildModelMatrix(
            cube_2.transform.position,
            cube_2.transform.scale,
            cube_2.transform.rotationDegrees - cubeAuto.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, cubeModel_2, view, projection, cube_2.overrideColor, cube_2.solidColor );
        //Rendering::DrawMesh(*cube_2.mesh); 
        Rendering::DrawMesh_FaceColors(shaderProgram, *cube_2.mesh, Demo::TestColors::cube_FaceColors); 
        
        // Cube version 2: placed behind the other cubes.
        glm::mat4 cubeModel_v2 = AcornMath::BuildModelMatrix(
            cube_V2.transform.position,
            cube_V2.transform.scale,
            cube_V2.transform.rotationDegrees + cubeAuto.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, cubeModel_v2, view, projection, cube_V2.overrideColor, cube_V2.solidColor );
        Rendering::DrawMesh(*cube_V2.mesh); 
        
        
        // Sphere: placed on the right.
        glm::mat4 sphereModel_1 = AcornMath::BuildModelMatrix(
            sphere_1.transform.position + sphereAuto.translation,
            sphere_1.transform.scale,
            sphere_1.transform.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, sphereModel_1, view, projection, sphere_1.overrideColor, sphere_1.solidColor);
        Rendering::DrawMesh(*sphere_1.mesh);
        
        glm::mat4 sphereModel_2 = AcornMath::BuildModelMatrix(
            sphere_2.transform.position - sphereAuto.translation,
            sphere_2.transform.scale,
            sphere_2.transform.rotationDegrees
        );
        Rendering::SetBasicShaderUniforms(shaderProgram, sphereModel_2, view, projection, sphere_2.overrideColor, sphere_2.solidColor);
        Rendering::DrawMesh(*sphere_2.mesh);
        
        
        
        glfwSwapBuffers(window);
    }
    
    
    
    // Mesh cleanup
    Rendering::DestroyGpuMesh(pyramidGpu);
    Rendering::DestroyGpuMesh(cubeGpu);
    Rendering::DestroyGpuMesh(cubeGpu_v2);
    Rendering::DestroyGpuMesh(sphereGpu);
    
    glDeleteProgram(shaderProgram.ID);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



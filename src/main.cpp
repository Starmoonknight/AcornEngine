// Project headers
#include "ShaderProgram.h" 

// Standard Library
#include <cassert>
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



namespace week1
{
    void CreateTriangleGeometry(unsigned int& VBO, unsigned int& VAO)
    {
        float vertices[] = {
            // positions         // colors
            0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right 
           -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left 
           -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // top 
       };  
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        CheckOpenGLErrors("triangle geometry buffer setup");
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
        glEnableVertexAttribArray(1);
        CheckOpenGLErrors("triangle geometry attribute setup");
    }
    
    
    void DrawTriangle(const ShaderProgram& shader, 
        glm::vec2 position, glm::vec2 scale, float rotationRad, 
        bool colorOverride = false, glm::vec3 color = glm::vec3(1.0f))
    {    
        int positionLocation = glGetUniformLocation(shader.ID, "uPosition");
        int scaleLocation = glGetUniformLocation(shader.ID, "uScale");
        int rotationLocation = glGetUniformLocation(shader.ID, "uRotation");
        
        int overrideLocation = glGetUniformLocation(shader.ID, "uColorOverride");
        int colorLocation = glGetUniformLocation(shader.ID, "uSolidColor");
        
        glm::vec3 position3D(position, 0.0f);
        glm::vec3 scale3D(scale, 1.0f);
        glm::vec3 rotation3D(0.0f, 0.0f, rotationRad);                      // needed to be updated to work with the newer 3D rotation                   
        
        // updates uniforms on the currently active program using the saved uniform locations 
        glUniform3fv(positionLocation, 1, glm::value_ptr(position3D));      // needed to be sent in as pointer instead of directly vector 
        glUniform3fv(scaleLocation, 1, glm::value_ptr(scale3D));
        glUniform3fv(rotationLocation, 1, glm::value_ptr(rotation3D));
        
        glUniform1i(overrideLocation, colorOverride);
        glUniform3fv(colorLocation, 1, glm::value_ptr(color));  
        CheckOpenGLErrors("setting triangle Uniform's");                        // Assumes the triangle's VAO is already bound.
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
        CheckOpenGLErrors("drawing triangle");
    }


    void DrawRectangle(const ShaderProgram& shader, 
        glm::vec2 position, glm::vec2 scale, float rotationRad, 
        bool colorOverride = false, glm::vec3 color = glm::vec3(1.0f))
    {
        DrawTriangle(shader, position, scale, rotationRad, colorOverride, color);
        DrawTriangle(shader, position, -scale, rotationRad, colorOverride, color);
    }
        
    void DrawHouse2D(const ShaderProgram& shader,  float testRotation, bool bHideRainbowRoofColor)
    {
        //house shape
        DrawRectangle(shader, glm::vec2(0.0f, -0.4f),  glm::vec2(1.2f,  0.8f), glm::radians(testRotation), true, Palette::orange); 
        DrawTriangle(shader, glm::vec2(0.0f, 0.0f),  glm::vec2(1.0f,  1.0f), glm::radians(-135.0f), bHideRainbowRoofColor, Palette::magenta); 
        //windows
        DrawRectangle(shader, glm::vec2(-0.25f, -0.2f), glm::vec2(0.3f, 0.2f), glm::radians(-testRotation), true, Palette::teal);
        DrawRectangle(shader, glm::vec2(0.25f, -0.2f), glm::vec2(0.3f, 0.2f), glm::radians(-testRotation), true, Palette::teal);
        // door
        DrawRectangle(shader, glm::vec2(0.0f, -0.6f),  glm::vec2(0.2f,  0.3f), glm::radians(testRotation), true, Palette::forestGreen);
        DrawRectangle(shader, glm::vec2(0.05f, -0.6f),  glm::vec2(0.05f,  0.05f), glm::radians(testRotation), true, Palette::black);
    }

    struct HouseRuntimeInfo
    {
        // runtime changing values 
        float testRotation = 0.0f;
        
        bool bPreviousSpacePress = false; 
        bool bHideRainbowRoofColor = true;
        
        bool bPreviousEnterPress = false; 
        bool bCrazyHouse = false; 
        
        
        bool bPreviousQPress = false; 
        bool bShowEyballs = false;
        
        float eyeMoveIncrement = 0.008f;
        float currentEyeMovement = 0.0f; 
        float maxRightMovement = 0.16f;
        float maxLeftMovement = -0.16f;
        bool bGoingRight = true;
    };

    void DrawCrazyHouse2D(GLFWwindow* window, const ShaderProgram& houseShader, HouseRuntimeInfo& house)
    {
        // key presses
        bool currentSpace = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool currentEnter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        bool currentQ = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
        
        if (currentSpace && !house.bPreviousSpacePress)
        {
            house.bHideRainbowRoofColor = !house.bHideRainbowRoofColor;
        }
        house.bPreviousSpacePress = currentSpace;
        
        if (house.bHideRainbowRoofColor == false && currentEnter && !house.bPreviousEnterPress)
        {
            house.bCrazyHouse = !house.bCrazyHouse;
        }
        house.bPreviousEnterPress = currentEnter;
        
        if (currentQ && !house.bPreviousQPress)
        {
            house.bShowEyballs = !house.bShowEyballs;
        }
        house.bPreviousQPress = currentQ;
        
        
        // house roof color changing, and eyeballs 
        if (house.bHideRainbowRoofColor == false)
        {
            if (house.bGoingRight == true)
            {
                if (house.currentEyeMovement > house.maxRightMovement)
                {
                    house.bGoingRight = false;
                }
                house.currentEyeMovement += house.eyeMoveIncrement;
            }
            else
            {
                if (house.currentEyeMovement < house.maxLeftMovement)
                {
                    house.bGoingRight = true;
                }
                house.currentEyeMovement -= house.eyeMoveIncrement;
            }
            
            if (house.bCrazyHouse == true)
            {
                house.testRotation += 0.6f;
                if (house.testRotation >= 360.0f)
                {
                    house.testRotation = 0.0f;
                }                
            }
            else
            {
                house.testRotation = 0.0f;
            }
        }
        else
        {
            house.currentEyeMovement = 0.0f;
            house.testRotation = 0.0f;
            house.bCrazyHouse = false;
        }
        
        
        DrawHouse2D(houseShader, house.testRotation, house.bHideRainbowRoofColor);
        
        //eyes
        if (house.bShowEyballs == true)
        {
            DrawRectangle(houseShader, glm::vec2(-0.25f + house.currentEyeMovement, -0.2f),  glm::vec2(0.09f,  0.09f), glm::radians(house.testRotation), true, Palette::white);
            DrawRectangle(houseShader, glm::vec2(-0.25f + house.currentEyeMovement, -0.2f),  glm::vec2(0.05f,  0.05f), glm::radians(house.testRotation), true, Palette::black);
            DrawRectangle(houseShader, glm::vec2(0.25f, -0.2f + house.currentEyeMovement),  glm::vec2(0.09f,  0.09f), glm::radians(house.testRotation), true, Palette::white);
            DrawRectangle(houseShader, glm::vec2(0.25f, -0.2f + house.currentEyeMovement),  glm::vec2(0.05f,  0.05f), glm::radians(house.testRotation), true, Palette::black);
        }
    }

}


namespace week2
{
    // maybe.... look into later what values to do save like this, for example color? 
    /*
    struct Vertex
    {
        float px, py, pz;
        float nx, ny, nz;
        float u, v; 
    };
    
    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices; 
    };
    */
    
    struct Pyramid
    {
        float pyramidVertices[5 * 6] = {
            // positions            // colors
            0.5f, -0.5f, -0.5f,     Palette::orange.r, Palette::orange.g, Palette::orange.b,    // 0, bottom, back right 
           -0.5f, -0.5f, -0.5f,     Palette::green.r, Palette::green.g, Palette::green.b,       // 1, bottom, back left 
           -0.5f, -0.5f, 0.5f,      Palette::pink.r, Palette::pink.g, Palette::pink.b,          // 2, bottom, forward left 
            0.5f, -0.5f, 0.5f,      Palette::red.r, Palette::red.g, Palette::red.b,             // 3, bottom, forward right
            0.0f, 0.5f, 0.0f,       Palette::blue.r, Palette::blue.g, Palette::blue.b           // 4, up, middle
       };  
        unsigned int pyramidIndices[18] = {
            // Pyramid: 18 indices
            2, 1, 0,    // bottom, half1
            0, 3, 2,    // bottom, half2
            0, 4, 3,    // right side face
            3, 4, 2,    // front right side
            2, 4, 1,    // left side face
            1, 4, 0,    // back side face
        };
    };
    
    struct Cube1
    {
        float cubeVertices[8 * 6] = {
            // positions            // colors
            0.5f, -0.5f, -0.5f,     Palette::orange.r, Palette::orange.g, Palette::orange.b,    // 0, bottom, back right 
           -0.5f, -0.5f, -0.5f,     Palette::green.r, Palette::green.g, Palette::green.b,       // 1, bottom, back left 
           -0.5f, -0.5f, 0.5f,      Palette::pink.r, Palette::pink.g, Palette::pink.b,          // 2, bottom, forward left 
            0.5f, -0.5f, 0.5f,      Palette::red.r, Palette::red.g, Palette::red.b,             // 3, bottom, forward right
        
            0.5f, 0.5f, -0.5f,     Palette::orange.r, Palette::orange.g, Palette::orange.b,     // 4, up, back right 
           -0.5f, 0.5f, -0.5f,     Palette::green.r, Palette::green.g, Palette::green.b,        // 5, up, back left 
           -0.5f, 0.5f, 0.5f,      Palette::pink.r, Palette::pink.g, Palette::pink.b,           // 6, up, forward left 
            0.5f, 0.5f, 0.5f,      Palette::red.r, Palette::red.g, Palette::red.b,              // 7, up, forward right
        }; 
        unsigned int cubeIndices[36] = {
            // Cube: 36 indices
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
    };
    
    struct Cube2
    {
        float cube2Vertices[24 * 6] = {
            // front face
            -0.5f, -0.5f, 0.5f,     1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.5f,     1.0f, 0.0f, 0.0f,
             0.5f,  0.5f, 0.5f,     1.0f, 1.0f, 0.0f,
            -0.5f,  0.5f, 0.5f,     1.0f, 1.0f, 0.0f,
        
            // Back face
            -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.0f,
             0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
        
            // Bottom face
            -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 0.0f,
        
            //Top face
            -0.5f, 0.5f,  -0.5f,    1.0f, 0.0f, 0.0f,
            -0.5f, 0.5f,   0.5f,    1.0f, 1.0f, 0.0f,
             0.5f, 0.5f,   0.5f,    1.0f, 1.0f, 0.0f,
             0.5f, 0.5f,  -0.5f,    1.0f, 0.0f, 0.0f,
        
            // Left face
            -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.0f,
        
            // Right face
            0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
            0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,
        };
    
        unsigned int cube2Indices[36] = {
            // Cube: 36 indices
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
    };
        
    
        // It's not the nicest sphere, nor the nicest code, but it sort of works.
    void generateSortofSphere(std::vector<float> &vertices, std::vector<unsigned int> &indices, int rings, int slices) {

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
    
    
    // expects 3 position attributes followed by 3 color attributes 
    void SetupGometryToRender(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO, 
        const float* vertices, GLsizeiptr verticesSizeBytes, const unsigned int* indices, GLsizeiptr indicesSizeBytes)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);      // selecting VAO to bind things to 
    
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verticesSizeBytes, vertices, GL_STATIC_DRAW);
        CheckOpenGLErrors("geometry buffer setup");
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSizeBytes, indices, GL_STATIC_DRAW);
        CheckOpenGLErrors("EBO buffer setup");
        
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
        glEnableVertexAttribArray(1);
        CheckOpenGLErrors("geometry attribute setup");
        
        glBindVertexArray(0); 
    }
    
    struct GeometryRuntimeValues_Transforms
    {   
        bool bPreviousPress_Space = false;
        
        // Scale
        bool bPreviousPress_Q = false;
        bool bPreviousPress_W = false;
        bool bPreviousPress_E = false;
    
        bool bScaleX = false;
        bool bScaleY = false;
        bool bScaleZ = false;
        
        glm::vec3 scale = glm::vec3(1.0f);
        
        bool bGrowingX = true;
        bool bGrowingY = true;
        bool bGrowingZ = true;
        
        // Rotation
        bool bPreviousPress_A = false;
        bool bPreviousPress_S = false;
        bool bPreviousPress_D = false;
    
        bool bRotateX = false;
        bool bRotateY = false;
        bool bRotateZ = false;
    
        glm::vec3 rotationDegrees = glm::vec3 (0.0f);
        
        // Location
        bool bPreviousPress_Z = false;
        bool bPreviousPress_X = false;
        bool bPreviousPress_C = false;
    
        bool bTranslateX = false;
        bool bTranslateY = false;
        bool bTranslateZ = false;
        
        glm::vec3 translation = glm::vec3 (0.0f);
        
        bool bPositivMovementX = true; 
        bool bPositivMovementY = true; 
        bool bPositivMovementZ = true; 
    };
    
    void HandleRuntimeInputs(GLFWwindow* window, GeometryRuntimeValues_Transforms& values)
    {
        bool current_Space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool bReset = false; 
        
        if (current_Space && !values.bPreviousPress_Space)
        {
            bReset = !bReset;
        }
        values.bPreviousPress_Space = current_Space;
        
        
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
        if (current_Q && !values.bPreviousPress_Q)
        {
            values.bScaleX = !values.bScaleX;
        }
        values.bPreviousPress_Q = current_Q;
        
        if (current_W && !values.bPreviousPress_W)
        {
            values.bScaleY = !values.bScaleY;
        }
        values.bPreviousPress_W = current_W;
        
        if (current_E && !values.bPreviousPress_E)
        {
            values.bScaleZ = !values.bScaleZ;
        }
        values.bPreviousPress_E = current_E;
        
        // rotation key inputs
        if (current_A && !values.bPreviousPress_A)
        {
            values.bRotateX = !values.bRotateX;
        }
        values.bPreviousPress_A = current_A;
        
        if (current_S && !values.bPreviousPress_S)
        {
            values.bRotateY = !values.bRotateY;
        }
        values.bPreviousPress_S = current_S;
        
        if (current_D && !values.bPreviousPress_D)
        {
            values.bRotateZ = !values.bRotateZ;
        }
        values.bPreviousPress_D = current_D;
        
        // translate key inputs
        if (current_Z && !values.bPreviousPress_Z)
        {
            values.bTranslateX = !values.bTranslateX;
        }
        values.bPreviousPress_Z = current_Z;
        
        if (current_X && !values.bPreviousPress_X)
        {
            values.bTranslateY = !values.bTranslateY;
        }
        values.bPreviousPress_X = current_X;
        
        if (current_C && !values.bPreviousPress_C)
        {
            values.bTranslateZ = !values.bTranslateZ;
        }
        values.bPreviousPress_C = current_C;        
        
        
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
    
    void UpdateShaderUniforms_OLD(const ShaderProgram& shader, 
        glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees, 
        bool colorOverride = false, glm::vec3 color = glm::vec3(1.0f))
    {
        int bUseMatrices = glGetUniformLocation(shader.ID, "uUseMatrices");
        
        int positionLocation = glGetUniformLocation(shader.ID, "uPosition");
        int scaleLocation = glGetUniformLocation(shader.ID, "uScale");
        int rotationLocation = glGetUniformLocation(shader.ID, "uRotation");
    
        int overrideLocation = glGetUniformLocation(shader.ID, "uColorOverride");
        int colorLocation = glGetUniformLocation(shader.ID, "uSolidColor");
        
        glUniform1i(bUseMatrices, false); 
        
        glm::vec3 rotationRad = glm::radians(rotationDegrees);                     // convert from angles in degrees to radians
    
        // updates uniforms on the currently active program using the saved uniform locations 
        glUniform3fv(positionLocation, 1, glm::value_ptr(position));      // needed to be sent in as pointer instead of directly vector 
        glUniform3fv(scaleLocation, 1, glm::value_ptr(scale));
        glUniform3fv(rotationLocation, 1, glm::value_ptr(rotationRad));
        
        glUniform1i(overrideLocation, colorOverride);
        glUniform3fv(colorLocation, 1, glm::value_ptr(color));  
        CheckOpenGLErrors("setting geometry Uniform's");                   
    }
    
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees)
    {
        // A starting matrix that makes no change.
        const glm::mat4 identity(1.0f);
        
        // Describe the object's size.
        glm::mat4 scaleMatrix = glm::scale(
            identity, 
            scale
        );
        
        // Describe rotation around the Y axis.
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
        
        // Describe rotation around the Y axis.
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
        
        // reads from right to left, applying scale, then rotation, then position
        glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        
        return modelMatrix;
    }
    
    void UpdateShaderUniforms_MATRIX(const ShaderProgram& shader, 
        glm::mat4 model, glm::mat4 view, glm::mat4 projection, 
        bool colorOverride = false, glm::vec3 color = glm::vec3(1.0f))
    {
        int bUseMatricesLocation = glGetUniformLocation(shader.ID, "uUseMatrices");
        
        int modelLocation = glGetUniformLocation(shader.ID, "uModel");
        int viewLocation = glGetUniformLocation(shader.ID, "uView");
        int projectionLocation = glGetUniformLocation(shader.ID, "uProjection");
    
        int bOverrideColorLocation = glGetUniformLocation(shader.ID, "uColorOverride");
        int colorLocation = glGetUniformLocation(shader.ID, "uSolidColor");
        
        glUniform1i(bUseMatricesLocation, GL_TRUE); 
        
        
        // updates uniforms on the currently active program using the saved uniform locations 
        glUniformMatrix4fv(modelLocation, 1 ,GL_FALSE,  glm::value_ptr(model));
        glUniformMatrix4fv(viewLocation, 1 ,GL_FALSE,  glm::value_ptr(view));
        glUniformMatrix4fv(projectionLocation, 1 ,GL_FALSE,  glm::value_ptr(projection));
        
        
        glUniform1i(bOverrideColorLocation, colorOverride);
        glUniform3fv(colorLocation, 1, glm::value_ptr(color));  
        CheckOpenGLErrors("setting geometry Uniform's");                   
    }
    
    
    void RenderPyramid()
    {        
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing pyramid");
    }
    
    void RenderCube()
    {        
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing cube");
    }    
    
    void RenderSphere(GLsizei indexCount) 
    {        
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing sphere");
    }
    
    
    void RenderPyramid_FullFaceColors(const ShaderProgram& shader)
    {
        int bOverrideColorLocation = glGetUniformLocation(shader.ID, "uColorOverride");
        int colorLocation = glGetUniformLocation(shader.ID, "uSolidColor");
        
        glUniform1i(bOverrideColorLocation, GL_TRUE);
        
        // has to match EBO draw order
        glm::vec3 faceColors[5] = {
            Palette::pink,    // Bottom
            Palette::yellow,  // Right
            Palette::red,     // Front
            Palette::green,   // Left
            Palette::blue,    // Back
        };
        
        std::size_t offsetBytes = 0;
        
        // draw one face each loop
        for (int face = 0; face < 5; ++face)
        {
            GLsizei indexCount;
            
            if (face == 0)
            {
                indexCount = 6; // Square bottom: two triangles.
            }
            else
            {
                indexCount = 3; // Each side: one triangle.
            }

            glUniform3fv(colorLocation, 1, glm::value_ptr(faceColors[face]));
            
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, reinterpret_cast<const void*>(offsetBytes));
            
            offsetBytes += static_cast<std::size_t>(indexCount) * sizeof(unsigned int);
        }
        
        glUniform1i(bOverrideColorLocation, GL_FALSE);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing pyramid with different face colors");
    }
    
    void RenderCube_FullFaceColors(const ShaderProgram& shader)
    {
        int bOverrideColorLocation = glGetUniformLocation(shader.ID, "uColorOverride");
        int colorLocation = glGetUniformLocation(shader.ID, "uSolidColor");
        
        glUniform1i(bOverrideColorLocation, GL_TRUE);
        
        // has to match EBO draw order
        glm::vec3 faceColors[6] = {
            Palette::red,     // Bottom
            Palette::orange,  // Right
            Palette::pink,    // Front
            Palette::green,   // Left
            Palette::blue,    // Back
            Palette::yellow  // Top
        };
        
        // draw six vertex points at a time in one color, making a single face each loop 
        for (int face = 0; face < 6; ++face)
        {
            glUniform3fv(colorLocation, 1, glm::value_ptr(faceColors[face]));
            
            std::size_t offsetBytes = face * 6 * sizeof(unsigned int);
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, reinterpret_cast<const void*>(offsetBytes));
        }
        
        glUniform1i(bOverrideColorLocation, GL_FALSE);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing cube with different face colors");
    }
    
}






int main(int argc, char **argv) {
    
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
    
    
    ShaderProgram ourShader("shaders/basic.vert", "shaders/basic.frag");
    if (!ourShader.IsValid())
    {
        std::cerr << "[Startup] Required shader failed; exiting.\n";
        
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    CheckOpenGLErrors("shader setup");
    
    
    
    unsigned int triangleVBO, triangleVAO;
    week1::CreateTriangleGeometry(triangleVBO, triangleVAO);
    
    
    
    // create pyramid 
    week2::Pyramid pyramid = {};
    unsigned int pyramidVAO, pyramidVBO, pyramidEBO;
    week2::SetupGometryToRender(pyramidVAO, pyramidVBO, pyramidEBO, 
        pyramid.pyramidVertices, sizeof(pyramid.pyramidVertices), pyramid.pyramidIndices, sizeof(pyramid.pyramidIndices));
    
    
    // create cube version 1  
    week2::Cube1 cube1 = {};
    unsigned int cubeVAO,  cubeVBO, cubeEBO;
    week2::SetupGometryToRender(cubeVAO, cubeVBO, cubeEBO,
        cube1.cubeVertices, sizeof(cube1.cubeVertices), cube1.cubeIndices, sizeof(cube1.cubeIndices));
    
    // create cube version 2 
    week2::Cube2 cube2 = {};
    unsigned int cube2VAO,  cube2VBO, cube2EBO;
    week2::SetupGometryToRender(cube2VAO, cube2VBO, cube2EBO,
        cube2.cube2Vertices, sizeof(cube2.cube2Vertices), cube2.cube2Indices, sizeof(cube2.cube2Indices));
    
    
    // create sphere
    std::vector<float> spherePositions; 
    std::vector<unsigned int> sphereIndices; 
    week2::generateSortofSphere(spherePositions, sphereIndices, 8, 16); 
    // convert the sphere vectors to the XYZ + RGB layout my other functions use 
    std::vector<float> sphereVertices;
    glm::vec3 sphereColor(1.0f, 0.5f, 0.0f); // Orange
    for (std::size_t i = 0; i < spherePositions.size(); i += 3)
    {
        // Position
        sphereVertices.push_back(spherePositions[i]);
        sphereVertices.push_back(spherePositions[i + 1]);
        sphereVertices.push_back(spherePositions[i + 2]);

        // Color
        sphereVertices.push_back(sphereColor.r);
        sphereVertices.push_back(sphereColor.g);
        sphereVertices.push_back(sphereColor.b);
    }
    
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    week2::SetupGometryToRender(sphereVAO, sphereVBO, sphereEBO,  
        sphereVertices.data(), static_cast<GLsizeiptr>(sphereVertices.size() * sizeof(float)),
        sphereIndices.data(), static_cast<GLsizeiptr>(sphereIndices.size() * sizeof(unsigned int))
    ); 
    
    GLsizei sphereIndexCount = static_cast<GLsizei>(sphereIndices.size());
    
    
    
    
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    glm::vec3 cameraPosition = glm::vec3(4.0f, 5.0f, 7.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
    
    
    
    // face culling
    GeometryRenderSettings renderSettings = {};
    glEnable(GL_CULL_FACE);
    renderSettings.bCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    
    // enable depth 
    glEnable(GL_DEPTH_TEST);
    
    
    // runtime values
    double previousTime = glfwGetTime();
    
    week1::HouseRuntimeInfo houseRuntimeInfo = {};
    renderSettings.bPerspectiveView = true; 
    
    // controlled by keyboard toggles.
    week2::GeometryRuntimeValues_Transforms runtimeInfo = {};
    // Automatic animations.
    week2::GeometryRuntimeValues_Transforms cubeAuto;
    cubeAuto.bRotateY = true;
    week2::GeometryRuntimeValues_Transforms sphereAuto;
    sphereAuto.bTranslateZ = true;
    week2::GeometryRuntimeValues_Transforms pyramidAuto;
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
        
        week2::HandleRuntimeInputs(window, runtimeInfo);

        week2::UpdateTransforms(runtimeInfo, deltaTime);
        week2::UpdateTransforms(cubeAuto, deltaTime);
        week2::UpdateTransforms(sphereAuto, deltaTime);
        week2::UpdateTransforms(pyramidAuto, deltaTime);
        
        
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
        ourShader.use(); 
        
        
        // triangle test, week 1
        //glBindVertexArray(triangleVAO);
        //week1::DrawCrazyHouse2D(window, ourShader, houseRuntimeInfo); 
        //glBindVertexArray(0);
        
        
        
        glm::vec3 scale = runtimeInfo.scale;
        glm::vec3 rotationDegrees = runtimeInfo.rotationDegrees;
        glm::vec3 movement = runtimeInfo.translation;
        
        
        // Pyramid: placed on the left.
        glm::mat4 pyramidModel_1 = week2::BuildModelMatrix(
            glm::vec3(-4.0f, 0.0f, 1.0f),
            glm::vec3(1.0f) * pyramidAuto.scale,
            glm::vec3(0.0f)
        );
        glBindVertexArray(pyramidVAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, pyramidModel_1, view, projection);
        week2::RenderPyramid(); 
        
        glm::mat4 pyramidModel_2 = week2::BuildModelMatrix(
            glm::vec3(-4.0f, 0.0f, -2.5f),
            glm::vec3(1.0f) * pyramidAuto.scale,
            glm::vec3(180.0f, 0.0f, 0.0f)
        );
        glBindVertexArray(pyramidVAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, pyramidModel_2, view, projection, true, Palette::blue);
        week2::RenderPyramid(); 
        
        
        // manually controlled pyramid
        glm::mat4 pyramidModel_Manual = week2::BuildModelMatrix(
            glm::vec3(0.0f, 0.0f, 4.0f) + movement,
            glm::vec3(1.0f) * scale,
            glm::vec3(0.0f, 0.0f, 0.0f) + rotationDegrees
        );
        glBindVertexArray(pyramidVAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, pyramidModel_Manual, view, projection);
        //week2::RenderPyramid(); 
        week2::RenderPyramid_FullFaceColors(ourShader); 
        
        
        // Cube version 1: placed in the middle.
        glm::mat4 cubeModel_1 = week2::BuildModelMatrix(
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(1.0f),
        glm::vec3(0.0f) + cubeAuto.rotationDegrees);
        glBindVertexArray(cubeVAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, cubeModel_1, view, projection);
        week2::RenderCube(); 
        
        glm::mat4 cubeModel_2 = week2::BuildModelMatrix(
            glm::vec3(0.0f, 0.0f, -2.5f),
            glm::vec3(1.0f),
            glm::vec3(0.0f) - cubeAuto.rotationDegrees);
        glBindVertexArray(cubeVAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, cubeModel_2, view, projection, true, Palette::pink);
        //week2::RenderCube(); 
        week2::RenderCube_FullFaceColors(ourShader); 
        
        // Cube version 2: placed behind the other cubes.
        glm::mat4 cube2Model = week2::BuildModelMatrix(
            glm::vec3(0.0f, 0.0f, -7.0f),
            glm::vec3(1.0f),
            glm::vec3(0.0f) + cubeAuto.rotationDegrees
        );
        glBindVertexArray(cube2VAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, cube2Model, view, projection);
        week2::RenderCube(); 
        
        /*        
        glBindVertexArray(cube2VAO);
        week2::UpdateShaderUniforms_OLD(ourShader, 
            glm::vec3(0.7f, 0.0f, 0.0f), // Position within the visible range
            glm::vec3(0.3f),                // Smaller cube
            glm::vec3(0.0f));
        week2::RenderCube(); 
        */
        
        
        // Sphere: placed on the right.
        glm::mat4 sphereModel_1 = week2::BuildModelMatrix(
            glm::vec3(3.0f, 0.0f, -1.0f) + sphereAuto.translation,
            glm::vec3(0.5f),
            glm::vec3(0.0f)
        );
        glBindVertexArray(sphereVAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, sphereModel_1, view, projection);
        week2::RenderSphere(sphereIndexCount);
        
        glm::mat4 sphereModel_2 = week2::BuildModelMatrix(
            glm::vec3(4.5f, 0.0f, -2.5f) - sphereAuto.translation,
            glm::vec3(0.5f),
            glm::vec3(0.0f)
        );
        glBindVertexArray(sphereVAO);
        week2::UpdateShaderUniforms_MATRIX(ourShader, sphereModel_2, view, projection, true, Palette::forestGreen);
        week2::RenderSphere(sphereIndexCount);
        
        
        
        glfwSwapBuffers(window);
    }
    
    
    
    // triangle cleanup
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVBO);
    
    // Pyramid cleanup
    glDeleteVertexArrays(1, &pyramidVAO);
    glDeleteBuffers(1, &pyramidVBO);
    glDeleteBuffers(1, &pyramidEBO);
    
    // Cube cleanup 
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    
    // Sphere cleanup
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    
    glDeleteProgram(ourShader.ID);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



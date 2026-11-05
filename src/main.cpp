// Project headers
#include "ShaderProgram.h" 

// Standard Library
#include <cassert>
#include <cmath>
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
        bool bPreviousTABPress = false;
        bool bwireframe = false;
        
        bool bPreviousCAPSPress = false;
        bool bCullFace = true;
    }; 
    
    void processInput_RenderSettings(GLFWwindow* window, GeometryRenderSettings& settings)
    {
        bool currentTAB = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (currentTAB && !settings.bPreviousTABPress)
        {
            settings.bwireframe = !settings.bwireframe;
            settings.bwireframe == true ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        // Set drawing mode to wireframe
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);        // Set drawing mode to filled, the default. 
        }
        settings.bPreviousTABPress = currentTAB;
        
        bool currentCAPS = glfwGetKey(window, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS;
        if (currentCAPS && !settings.bPreviousCAPSPress)
        {
            settings.bCullFace = (glIsEnabled(GL_CULL_FACE) == GL_TRUE);        // to make sure it is not getting out of sync with the bool meaning 
            settings.bCullFace = !settings.bCullFace;
            
            settings.bCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        }
        settings.bPreviousCAPSPress = currentCAPS;
    }
    
    
}





namespace Palette
{
    // Colours
    const glm::vec3 red         {1.0f,  0.0f,  0.0f};
    const glm::vec3 magenta     {1.0f,  0.0f,  1.0f};
    const glm::vec3 pink        {1.0f,  0.75f, 0.8f};
    const glm::vec3 orange      {1.0f,  0.5f,  0.0f};

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
    void SetupGometryToRender(unsigned int& VBO, unsigned int& VAO, unsigned int& EBO, 
        const float* vertices, GLsizeiptr verticesSizeBytes, const unsigned int* indices, GLsizeiptr indicesSizeBytes)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);      // selecting VAO yo bind things to 
    
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
    }
    
    struct PyramidRuntimeValues
    {    
        bool bPreviousQPress = false;
        bool bPreviousWPress = false;
        bool bPreviousEPress = false;
    
        bool bRotateX = false;
        bool bRotateY = false;
        bool bRotateZ = false;
    
        float rotationDegreesX = 0.0f;
        float rotationDegreesY = 0.0f;
        float rotationDegreesZ = 0.0f;
    };
    
    void HandleRuntimeInputs(GLFWwindow* window, PyramidRuntimeValues& runtimeValues, float deltaTime)
    {
        bool currentQ = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
        bool currentW = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool currentE = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        
        if (currentQ && !runtimeValues.bPreviousQPress)
        {
            runtimeValues.bRotateX = !runtimeValues.bRotateX;
        }
        runtimeValues.bPreviousQPress = currentQ;
        
        if (currentW && !runtimeValues.bPreviousWPress)
        {
            runtimeValues.bRotateY = !runtimeValues.bRotateY;
        }
        runtimeValues.bPreviousWPress = currentW;
        
        if (currentE && !runtimeValues.bPreviousEPress)
        {
            runtimeValues.bRotateZ = !runtimeValues.bRotateZ;
        }
        runtimeValues.bPreviousEPress = currentE;
        
        
        constexpr float DegreesPerSecond = 90.0f;
        if (runtimeValues.bRotateX)
        {
            runtimeValues.rotationDegreesX = std::fmod(
                runtimeValues.rotationDegreesX + DegreesPerSecond * deltaTime,
                360.0f
            );
        }
        if (runtimeValues.bRotateY)
        {
            runtimeValues.rotationDegreesY = std::fmod(
                runtimeValues.rotationDegreesY + DegreesPerSecond * deltaTime,
                360.0f
            );
        }
        if (runtimeValues.bRotateZ)
        {
            runtimeValues.rotationDegreesZ = std::fmod(
                runtimeValues.rotationDegreesZ + DegreesPerSecond * deltaTime,
                360.0f
            );
        }        
    }
    
    void RenderPyramid(const ShaderProgram& shader, 
    glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees, 
    bool colorOverride = false, glm::vec3 color = glm::vec3(1.0f))
    {
        int positionLocation = glGetUniformLocation(shader.ID, "uPosition");
        int scaleLocation = glGetUniformLocation(shader.ID, "uScale");
        int rotationLocation = glGetUniformLocation(shader.ID, "uRotation");
    
        int overrideLocation = glGetUniformLocation(shader.ID, "uColorOverride");
        int colorLocation = glGetUniformLocation(shader.ID, "uSolidColor");
        
        glm::vec3 rotationRad = glm::radians(rotationDegrees);                     // convert from angles in degrees to radians
    
        // updates uniforms on the currently active program using the saved uniform locations 
        glUniform3fv(positionLocation, 1, glm::value_ptr(position));      // needed to be sent in as pointer instead of directly vector 
        glUniform3fv(scaleLocation, 1, glm::value_ptr(scale));
        glUniform3fv(rotationLocation, 1, glm::value_ptr(rotationRad));
        
        glUniform1i(overrideLocation, colorOverride);
        glUniform3fv(colorLocation, 1, glm::value_ptr(color));  
        CheckOpenGLErrors("setting pyramid Uniform's");                        // Assumes the triangle's VAO is already bound.
        
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        CheckOpenGLErrors("drawing pyramid");
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
    
    
    
    float pyramidVertices[] = {
        // positions            // colors
        0.5f, -0.5f, -0.5f,     Palette::orange.r, Palette::orange.g, Palette::orange.b,    // 0, bottom, back right 
       -0.5f, -0.5f, -0.5f,     Palette::green.r, Palette::green.g, Palette::green.b,       // 1, bottom, back left 
       -0.5f, -0.5f, 0.5f,      Palette::pink.r, Palette::pink.g, Palette::pink.b,          // 2, bottom, forward left 
        0.5f, -0.5f, 0.5f,      Palette::red.r, Palette::red.g, Palette::red.b,             // 3, bottom, forward right
        0.0f, 0.5f, 0.0f,       Palette::blue.r, Palette::blue.g, Palette::blue.b           // 4, up, middle
   };  
    unsigned int pyramidIndices[] = {
        // Pyramid: 18 indices
        2, 1, 0,    // bottom, half1
        0, 3, 2,    // bottom, half2
        0, 4, 3,    // right side face
        3, 4, 2,    // front right side
        2, 4, 1,    // left side face
        1, 4, 0,    // back side face
    };
    
    unsigned int pyramidVBO, pyramidVAO, pyramidEBO;
    glGenVertexArrays(1, &pyramidVAO);
    glGenBuffers(1, &pyramidVBO);
    glGenBuffers(1, &pyramidEBO);
    glBindVertexArray(pyramidVAO);      // selecting VAO yo bind things to 
    
    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);
    CheckOpenGLErrors("geometry buffer setup");
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pyramidEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pyramidIndices), pyramidIndices, GL_STATIC_DRAW);
    CheckOpenGLErrors("EBO buffer setup");
    
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);
    CheckOpenGLErrors("geometry attribute setup");
    
    
    
    float cubeVertices[] = {
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
    
    unsigned int cubeIndices[] = {
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
    
    
    
    // face culling
    GeometryRenderSettings renderSettings = {};
    
    glEnable(GL_CULL_FACE);
    renderSettings.bCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    
    
    // runtime values
    double previousTime = glfwGetTime();
    
    week1::HouseRuntimeInfo houseRuntimeInfo = {};
    week2::PyramidRuntimeValues pyramidRuntimeInfo = {};
    
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        processInput_ESCAPE(window);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;
                
        processInput_RenderSettings(window, renderSettings);
        week2::HandleRuntimeInputs(window, pyramidRuntimeInfo, deltaTime);
        
        
        // activate program
        ourShader.use(); 
        
        // triangle test
        //glBindVertexArray(triangleVAO);
        //week1::DrawCrazyHouse2D(window, ourShader, houseRuntimeInfo); 
        //glBindVertexArray(0);
        
        // Pyramid test
        glBindVertexArray(pyramidVAO);
        CheckOpenGLErrors("frame clear, shader activation and VAO binding");
        week2::RenderPyramid(
            ourShader, glm::vec3(0), glm::vec3(1.0), 
            glm::vec3(pyramidRuntimeInfo.rotationDegreesX, pyramidRuntimeInfo.rotationDegreesY, pyramidRuntimeInfo.rotationDegreesZ));
        
        
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    
    
    // triangle cleanup
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVBO);
    
    // Pyramid cleanup
    glDeleteVertexArrays(1, &pyramidVAO);
    glDeleteBuffers(1, &pyramidVBO);
    glDeleteBuffers(1, &pyramidEBO);
    glDeleteProgram(ourShader.ID);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



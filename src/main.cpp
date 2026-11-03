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
    
    
    void processInput(GLFWwindow* window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
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
    
    // updates uniforms on the currently active program using the saved uniform locations 
    glUniform3fv(positionLocation, 1, glm::value_ptr(position3D));      // needed to be sent in as pointer instead of directly vector 
    glUniform3fv(scaleLocation, 1, glm::value_ptr(scale3D));
    glUniform1f(rotationLocation, rotationRad);
    
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
    
    
    float vertices[] = {
        // positions         // colors
        0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right 
       -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left 
       -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // top 
   };  
    
    unsigned int VBO, VAO;
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
    
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        // Set drawing mode to wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);        // Set drawing mode to filled, the default. 
    
    // face culling
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    
    
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
    
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        
        // key presses
        bool currentSpace = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool currentEnter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        bool currentQ = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
        
        if (currentSpace && !bPreviousSpacePress)
        {
            bHideRainbowRoofColor = !bHideRainbowRoofColor;
        }
        bPreviousSpacePress = currentSpace;
        
        if (bHideRainbowRoofColor == false && currentEnter && !bPreviousEnterPress)
        {
            bCrazyHouse = !bCrazyHouse;
        }
        bPreviousEnterPress = currentEnter;
        
        if (currentQ && !bPreviousQPress)
        {
            bShowEyballs = !bShowEyballs;
        }
        bPreviousQPress = currentQ;
        
        
        // house roof color changing, and eyeballs 
        if (bHideRainbowRoofColor == false)
        {
            if (bGoingRight == true)
            {
                if (currentEyeMovement > maxRightMovement)
                {
                    bGoingRight = false;
                }
                currentEyeMovement += eyeMoveIncrement;
            }
            else
            {
                if (currentEyeMovement < maxLeftMovement)
                {
                    bGoingRight = true;
                }
                currentEyeMovement -= eyeMoveIncrement;
            }
            
            if (bCrazyHouse == true)
            {
                testRotation += 0.6f;
                if (testRotation >= 360.0f)
                {
                    testRotation = 0.0f;
                }                
            }
            else
            {
                testRotation = 0.0f;
            }
        }
        else
        {
            currentEyeMovement = 0.0f;
            testRotation = 0.0f;
            bCrazyHouse = false;
        }
        
        
        // activate program
        ourShader.use(); 
        
        // Triangle test
        glBindVertexArray(VAO);
        CheckOpenGLErrors("frame clear, shader activation and VAO binding");
        
        
        //house shape
        DrawRectangle(ourShader, glm::vec2(0.0f, -0.4f),  glm::vec2(1.2f,  0.8f), glm::radians(testRotation), true, Palette::orange); 
        DrawTriangle(ourShader, glm::vec2(0.0f, 0.0f),  glm::vec2(1.0f,  1.0f), glm::radians(-135.0f), bHideRainbowRoofColor, Palette::magenta); 
        //windows
        DrawRectangle(ourShader, glm::vec2(-0.25f, -0.2f), glm::vec2(0.3f, 0.2f), glm::radians(-testRotation), true, Palette::teal);
        DrawRectangle(ourShader, glm::vec2(0.25f, -0.2f), glm::vec2(0.3f, 0.2f), glm::radians(-testRotation), true, Palette::teal);
        // door
        DrawRectangle(ourShader, glm::vec2(0.0f, -0.6f),  glm::vec2(0.2f,  0.3f), glm::radians(testRotation), true, Palette::forestGreen);
        DrawRectangle(ourShader, glm::vec2(0.05f, -0.6f),  glm::vec2(0.05f,  0.05f), glm::radians(testRotation), true, Palette::black);
        
        //eyes
        if (bShowEyballs == true)
        {
            DrawRectangle(ourShader, glm::vec2(-0.25f + currentEyeMovement, -0.2f),  glm::vec2(0.09f,  0.09f), glm::radians(testRotation), true, Palette::white);
            DrawRectangle(ourShader, glm::vec2(-0.25f + currentEyeMovement, -0.2f),  glm::vec2(0.05f,  0.05f), glm::radians(testRotation), true, Palette::black);
            DrawRectangle(ourShader, glm::vec2(0.25f, -0.2f + currentEyeMovement),  glm::vec2(0.09f,  0.09f), glm::radians(testRotation), true, Palette::white);
            DrawRectangle(ourShader, glm::vec2(0.25f, -0.2f + currentEyeMovement),  glm::vec2(0.05f,  0.05f), glm::radians(testRotation), true, Palette::black);
        }
        
        
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    // Triangle cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(ourShader.ID);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



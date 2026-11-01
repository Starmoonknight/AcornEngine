#include <cassert>
#include <cstddef>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include <cmath>

#include "Shader.h" 


namespace
{
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    
    void glfwErrorCallback(int error, const char* description)
    {
        std::cerr << "GLFW error (" << error << "): " << description << '\n';
    }
    
    void framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height); 
    }
    
    
    bool InitializeProgram()
    {
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
            return window;
        }
    
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


void DrawTriangle(Shader shader, 
                  float posX, float posY, 
                  float scaleX, float scaleY, 
                  float rotation,
                  bool colorOverride = false, float red = 1, float green = 1, float blue = 1)
{
    
    int positionLocation = glGetUniformLocation(shader.ID, "uPosition");
    int scaleLocation = glGetUniformLocation(shader.ID, "uScale");
    int rotationLocation = glGetUniformLocation(shader.ID, "uRotation");
    
    int overrideLocation = glGetUniformLocation(shader.ID, "uColorOverride");
    int colorLocation = glGetUniformLocation(shader.ID, "uSolidColor");
    
    glUniform3f(positionLocation, posX, posY, 1.0f);
    glUniform3f(scaleLocation, scaleX, scaleY, 1.0f);
    glUniform1f(rotationLocation, rotation);
    
    glUniform1i(overrideLocation, colorOverride);
    glUniform3f(colorLocation, red, green, blue);
    
    
    
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
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
    
    Shader ourShader("shaders/basic.vert", "shaders/basic.frag");
    
    
    float vertices[] = {
        // positions         // colors
        0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right 
       -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left 
        0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
   };  
    
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);
    
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        // Set drawing mode to wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);        // Set drawing mode to filled, the default. 
    
    // face culling
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // activate program
        ourShader.use(); 
        
        // Triangle test
        glBindVertexArray(VAO);
        DrawTriangle(ourShader, 0.0f, 0.0f,  1.0f,  1.0f, 0.0f);    // rainbow triangle 
        //DrawTriangle(ourShader, 0.0f, 0.0f,  1.0f,  1.0f, 0.0f, true, 0.2f, 0.0f, 0.3f);  // purple triangle
        
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



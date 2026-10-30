#include <cassert>
#include <cstddef>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include <cmath>


namespace
{
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    
    const char *vertexShaderSource = "#version 460 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
    
        "out vec3 ourColor;\n"
        
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos, 1.0);\n"
        "   ourColor = aColor;\n" 
        "}\0";
    const char *fragmentShaderSource = "#version 460 core\n"
        "out vec4 FragColor;\n"
        "in vec3 ourColor;\n"
        
        "void main()\n"
        "{\n"
        "   FragColor = vec4(ourColor, 1.0);\n"
        "}\n\0";
    
    
    
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
    
    
    // Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check if compilation was successful 
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check if compilation was successful 
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check if compilation was successful 
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) 
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    // cleanup of used shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
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
    
    glBindVertexArray(VAO);
    
    
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
        glUseProgram(shaderProgram);
        
        // Triangle test
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    // Triangle cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


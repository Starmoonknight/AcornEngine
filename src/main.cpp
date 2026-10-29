#include <cassert>
#include <cstddef>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>


namespace
{
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    
    const char *vertexShaderSource = "#version 460 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";
    const char *fragmentShaderSource = "#version 460 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
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
    
    // don't think I like grouping together these in a struct, because multiple combinations are possible, 
    // it's too specific and has potential ownership issues later on. Will keep for now as learning history. 
    struct TriangleGeometry
    {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
    };
    
    // moved this out to get a better overview of the steps, but feels like the function is to rigid 
    TriangleGeometry CreateTriangleGeometry()
    {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };  
        
        TriangleGeometry triangle{};
        
        glGenVertexArrays(1, &triangle.VAO);
        glGenBuffers(1, &triangle.VBO);  
        
        glBindVertexArray(triangle.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, triangle.VBO); 
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(
            0,                   // Attribute location.
            3,                   // x, y, z.
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),   // Distance between positions.
            (void*)0             // Starting offset.
        );
        glEnableVertexAttribArray(0);
        
        // unbind VBO and VAO to clear what is being selected 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return triangle;
    }
    
    void DestroyTriangleGeometry(TriangleGeometry& triangle)
    {
        glDeleteVertexArrays(1, &triangle.VAO);
        glDeleteBuffers(1, &triangle.VBO);

        triangle = {};
    }
    
    
    
    // second iteration of the CreateTriangleGeometry() function, goal to be more generic and reusable, 
    // but this is still probably a bad or at least not well-made function  
    // also found out assert, that looks useful
    void CreatePositionOnlyGeometry(
        unsigned int& VAO, unsigned int& VBO,
        const float* vertices, std::size_t vertexDataBytes, 
        int componentsPerPosition = 3,
        GLenum usage = GL_STATIC_DRAW)
    {
        // testing out assert preconditions 
        assert(VAO == 0 && VBO == 0);
        assert(vertices != nullptr);
        assert(componentsPerPosition >= 2 && componentsPerPosition <= 4);
        
        const std::size_t positionBytes = static_cast<std::size_t>(componentsPerPosition) * sizeof(float);
        
        assert(vertexDataBytes > 0);
        assert(vertexDataBytes % positionBytes == 0);
        
        const GLsizeiptr uploadBytes = static_cast<GLsizeiptr>(vertexDataBytes);
        const GLsizei stride = static_cast<GLsizei>(positionBytes);
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);  
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO); 
        
        glBufferData(GL_ARRAY_BUFFER, uploadBytes, vertices, usage);
        
        glVertexAttribPointer(
            0,                      // Attribute location.
            componentsPerPosition,  // x, y, z.
            GL_FLOAT,
            GL_FALSE,
            stride,                 // Distance between positions.
            nullptr                 // Starting offset.
        );
        glEnableVertexAttribArray(0);
        
        // unbind VBO and VAO to clear what is being selected, no relying on selections surviving between functions 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
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
    
    
    // Triangle test 
    //TriangleGeometry triangle = CreateTriangleGeometry();
    
    
    // Rectangle test
    float vertices[] = {
        0.5f,  0.5f, 0.0f,  // top right
        0.5f, -0.5f, 0.0f,  // bottom right
       -0.5f, -0.5f, 0.0f,  // bottom left
       -0.5f,  0.5f, 0.0f   // top left 
   };
    unsigned int indices[] = {  
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };  
    
    unsigned int VAO = 0;
    unsigned int VBO = 0;

    CreatePositionOnlyGeometry(VAO, VBO, vertices, sizeof(vertices));
    
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 
    
    glBindVertexArray(0);
    
    
    
    
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    
        // activate program
        glUseProgram(shaderProgram);
        
        // Triangle test
        //glBindVertexArray(triangle.VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        
        // Rectangle test
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    
    //DestroyTriangleGeometry(triangle);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    // also delete EBO? fix next day 
    
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


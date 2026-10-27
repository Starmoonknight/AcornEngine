#include <iostream>
#include <glfw/glfw3.h>
#include <glad/glad.h>


int main(int argc, char **argv) {
    
    if (!glfwInit())
    {
        std::cout << "you did glfwInit wrong!" << '\n';
    }
    
    std::cout << "hello world!" << std::endl;
    return 0;
    
    
    
}

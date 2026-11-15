#pragma once

// Standard Library
#include <cassert>  
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// Third-party libraries
#include <glad/glad.h>      // include glad to get all the required OpenGL headers
#include <glm/glm.hpp>



struct BasicUniformLocations
{
    GLint modelLocation = -1;
    GLint viewLocation = -1;
    GLint projectionLocation = -1;

    GLint overrideColorLocation = -1;
    GLint colorLocation = -1;
    GLint colorAmountLocation = -1;
    
    GLint useTextureLocation = -1;
    GLint textureLocation = -1;
    GLint tintTextureColorAmountLocation = -1;
};


class ShaderProgram
{
public:
    unsigned int ID = 0;
    
    ShaderProgram(const char* vertexPath, const char* fragmentPath)
    { 
        // retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        
        // opening and reading the shader files 
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch(std::ifstream::failure& e)
        {
            std::cerr << "[Shader][File] Failed to read shader source.\n"
                    << "Vertex path: " << vertexPath << '\n'
                    << "Fragment path: " << fragmentPath << '\n'
                    << "Details: " << e.what() << '\n';
            
            return;  // returns with a false valid bool after failing init 
        }
        
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        
        
        
        
        unsigned int vertex, fragment;
        int success = 0;
        char infoLog[512];
        
        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        if (vertex == 0)
        {
            std::cerr << "[Shader] Failed to create vertex shader.\n";
            return;
        }        
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        // check if compilation was successful 
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            // note: compilation failiure is not itself an error, so not using glGetError() 
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            
            glDeleteShader(vertex);
            return;
        }
        
        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        if (fragment == 0)
        {
            std::cerr << "[Shader] Failed to create fragment shader.\n";

            glDeleteShader(vertex);
            return;
        }
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        // check if compilation was successful 
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
            
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            return;
        }
        
        // Shader Program
        ID = glCreateProgram();
        if (ID == 0)
        {
            std::cerr << "[Shader] Failed to create shader program.\n";

            glDeleteShader(vertex);
            glDeleteShader(fragment);
            return;
        }
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        // check if compilation was successful 
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success) 
        {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            glDeleteProgram(ID);
            ID = 0; 
            return;
        }
        // cleanup of used shaders after linking 
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        
        valid = true;
    }
    
    
    
    void use() const
    {
        glUseProgram(ID);
    }
    
    bool IsValid() const
    {
        return valid;
    }
    
    // utility uniform functions    - LearnOpenGL styled 
    // ------------------------------------------------------------------------
    void SetBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
    }
    // ------------------------------------------------------------------------
    void SetInt(const std::string &name, int value) const 
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
    }
    // ------------------------------------------------------------------------
    void SetFloat(const std::string &name, float value) const 
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const
    { 
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y) const
    { 
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const
    { 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    { 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) const
    { 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    
    
    // Alternative utility uniform functions
    // ------------------------------------------------------------------------
    void CacheBasicUniformLocations()
    {
        assert(IsValid());

        basicUniforms.modelLocation =
            glGetUniformLocation(ID, "uModel");

        basicUniforms.viewLocation =
            glGetUniformLocation(ID, "uView");

        basicUniforms.projectionLocation =
            glGetUniformLocation(ID, "uProjection");

        
        basicUniforms.overrideColorLocation =
            glGetUniformLocation(ID, "uColorOverride");

        basicUniforms.colorLocation =
            glGetUniformLocation(ID, "uObjectColor");

        basicUniforms.colorAmountLocation =
            glGetUniformLocation(ID, "uColorAmount");
        
        
        basicUniforms.useTextureLocation =
            glGetUniformLocation(ID, "uUseTexture");
        
        basicUniforms.textureLocation =
            glGetUniformLocation(ID, "uTexture2D");
        
        basicUniforms.tintTextureColorAmountLocation =
            glGetUniformLocation(ID, "uTintTextureColorAmount");
        

        assert(basicUniforms.modelLocation != -1);
        assert(basicUniforms.viewLocation != -1);
        
        assert(basicUniforms.projectionLocation != -1);
        assert(basicUniforms.overrideColorLocation != -1);
        assert(basicUniforms.colorLocation != -1);
        
        assert(basicUniforms.useTextureLocation != -1);
        assert(basicUniforms.textureLocation != -1);
        assert(basicUniforms.tintTextureColorAmountLocation != -1);
    }
    
    const BasicUniformLocations& GetBasicUniformLocations() const
    {
        return basicUniforms;
    }
    
    
    
private:
    bool valid = false;
    BasicUniformLocations basicUniforms{};
    
    
    // not in use yet, switch the above success checks with this later. 
    bool checkCompileErrors(unsigned int shader, std::string type)
    {
        int success = 0;
        char infoLog[1024] = {};
        
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, sizeof(infoLog), NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        
        // true means operation succedes, failed means error was found 
        return success == GL_TRUE;
    }
};


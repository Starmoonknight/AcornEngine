// Project headers
#include "AcornMath.h"
#include "assets/ModelUpload.h"
#include "geometry/BasicMeshes.h"
#include "geometry/CpuMeshData.h"
#include "rendering/OpenGLErrors.h"
#include "rendering/GpuModel.h"
#include "rendering/Renderer.h"
#include "rendering/RenderInstances.h"
#include "rendering/ShaderProgram.h" 
#include "Palette.h" 


// Standard library
#include <cassert>      // assert 
#include <cmath>        // Scalar math for geometry and animation
#include <iostream>     // std::cout and std::cerr for diagnostics
#include <vector>       // Mesh data and temporary OBJ attribute lists


// Third-party libraries - OpenGL
#include <glad/glad.h>                      // Keep glad include before GLFW include 
#include <glfw/glfw3.h>                     // Window, context, input, and timing

// Third-party libraries - GLM
#include <glm/glm.hpp>                      // Vectors, matrices, dot and cross products
#include <glm/gtc/matrix_transform.hpp>     // Model, view, and projection transforms

// Third-party libraries - stb 
#include "stb_image.h"                      // Texture image loading and vertical flip




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




namespace AcornData
{
    
    
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
        using CpuMeshData = AcornData::CpuMeshData;
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::ModelInstance;
        
        
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
        CpuMeshData pyramidData = BasicMeshes::MakePyramid_Basic();
        CpuMeshData cubeData = BasicMeshes::MakeCube_Basic();
        CpuMeshData sphereData = BasicMeshes::MakeBasicSphere();
        
        CpuMeshData pyramidData_v2 = BasicMeshes::MakeDefaultPyramid();
        CpuMeshData cubeData_v2 = BasicMeshes::MakeDefaultCube();
        
        
        GpuModel pyramidGpu = Rendering::CreateGpuModel(pyramidData);
        GpuModel cubeGpu = Rendering::CreateGpuModel(cubeData);
        GpuModel sphereGpu = Rendering::CreateGpuModel(sphereData);
        
        GpuModel pyramidGp_v2 = Rendering::CreateGpuModel(pyramidData_v2);
        GpuModel cubeGpu_v2 = Rendering::CreateGpuModel(cubeData_v2);
        
    
        // making object instances
        ModelInstance basicPyramid_1{&pyramidGpu, Transform{glm::vec3(-4.0f, 0.0f, 1.0f)}};
        ModelInstance basicPyramid_2{&pyramidGpu, Transform{glm::vec3(-4.0f, 0.0f, -2.5f), glm::vec3(180.0f, 0.0f, 0.0f)}, true, Palette::blue};
        
        ModelInstance pyramid_V1{&pyramidGp_v2, Transform{glm::vec3(-4.0f, 0.0f, -6.0f)}};
        ModelInstance pyramid_manual{&pyramidGp_v2, Transform{glm::vec3(0.0f, 0.0f, 4.0f)}};
    
        
        ModelInstance basicCube_1{&cubeGpu, Transform{glm::vec3(0.0f, 0.0f, 1.0f)}};
        ModelInstance basicCube_2{&cubeGpu, Transform{glm::vec3(0.0f, 0.0f, -2.5f)}, true, Palette::pink};
        
        ModelInstance cube_V2{&cubeGpu_v2, Transform{glm::vec3(0.0f, 0.0f, -7.0f)}};
        
        
        ModelInstance sphere_1{&sphereGpu, Transform{glm::vec3(3.0f, 0.0f, -1.0f)}};
        ModelInstance sphere_2{&sphereGpu, Transform{glm::vec3(4.5f, 0.0f, -2.5f)}, true, Palette::forestGreen};
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
        glm::mat4 pyramidMatrix_1 = AcornMath::BuildModelMatrix(
            basicShapes.basicPyramid_1.transform.position,
            basicShapes.basicPyramid_1.transform.scale * basicShapes.pyramidAuto.scale,
            basicShapes.basicPyramid_1.transform.rotationDegrees
        );
        Rendering::DrawModel(
            shaderProgram, view, projection, basicShapes.basicPyramid_1, pyramidMatrix_1,
            basicShapes.basicPyramid_1.textureID, false, 1.0f, 0.0f
        );
        
        glm::mat4 pyramidMatrix_2 = AcornMath::BuildModelMatrix(
            basicShapes.basicPyramid_2.transform.position,
            basicShapes.basicPyramid_2.transform.scale * basicShapes.pyramidAuto.scale,
            basicShapes.basicPyramid_2.transform.rotationDegrees
        );       
        Rendering::DrawModel(
            shaderProgram, view, projection, basicShapes.basicPyramid_2, pyramidMatrix_2,
            basicShapes.basicPyramid_2.textureID, false, 1.0f, 0.0f
        );
        
        
        glm::mat4 pyramidMatrix_v2 = AcornMath::BuildModelMatrix(
            basicShapes.pyramid_V1.transform.position,
            basicShapes.pyramid_V1.transform.scale * basicShapes.pyramidAuto.scale,
            basicShapes.pyramid_V1.transform.rotationDegrees
        );
        Rendering::DrawModel(
            shaderProgram, view, projection, basicShapes.pyramid_V1, pyramidMatrix_v2,
            basicShapes.pyramid_V1.textureID, true, 1.0f, 0.0f
        );
        
        
        // manually controlled pyramid,     testing drawing colors per face 
        glm::mat4 pyramidMatrix_Manual = AcornMath::BuildModelMatrix(
            basicShapes.pyramid_manual.transform.position + manualPosition,
            basicShapes.pyramid_manual.transform.scale * manualScale,
            basicShapes.pyramid_manual.transform.rotationDegrees + manualRotationDegrees
        );
        
        assert(basicShapes.pyramid_manual.gpuModel != nullptr);
        assert(basicShapes.pyramid_manual.gpuModel->meshes.size() == 1);
        
        const auto& manualPyramidMesh = basicShapes.pyramid_manual.gpuModel->meshes.front();
        
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidMatrix_Manual, view, projection, 
            basicShapes.pyramid_manual.overrideColor, basicShapes.pyramid_manual.solidColor, 1.0f, false, 0.0f
        );
        shaderProgram.SetBool("uUseLighting", manualPyramidMesh.hasNormals);
        Rendering::DrawMesh_FaceColors(shaderProgram, manualPyramidMesh, Demo::TestColors::pyramid_FaceColors); 
        
        
        
        // Cube version 1: placed in the middle.
        glm::mat4 cubeMatrix_1 = AcornMath::BuildModelMatrix(
            basicShapes.basicCube_1.transform.position,
            basicShapes.basicCube_1.transform.scale,
            basicShapes.basicCube_1.transform.rotationDegrees + basicShapes.cubeAuto.rotationDegrees
        );
        Rendering::DrawModel(
            shaderProgram, view, projection, basicShapes.basicCube_1, cubeMatrix_1,
            basicShapes.basicCube_1.textureID, false, 1.0f, 0.0f
        );
        
        
        // testing drawing colors per face 
        glm::mat4 cubeMatrix_2 = AcornMath::BuildModelMatrix(
            basicShapes.basicCube_2.transform.position,
            basicShapes.basicCube_2.transform.scale,
            basicShapes.basicCube_2.transform.rotationDegrees - basicShapes.cubeAuto.rotationDegrees
        );
        
        assert(basicShapes.basicCube_2.gpuModel != nullptr);
        assert(basicShapes.basicCube_2.gpuModel->meshes.size() == 1);
        
        const auto& faceColoredCubeMesh = basicShapes.basicCube_2.gpuModel->meshes.front();
        
        Rendering::SetBasicShaderUniforms(shaderProgram, cubeMatrix_2, view, projection, 
            basicShapes.basicCube_2.overrideColor, basicShapes.basicCube_2.solidColor, 1.0f, false, 0.0f
        );
        shaderProgram.SetBool("uUseLighting", false);
        Rendering::DrawMesh_FaceColors(shaderProgram, faceColoredCubeMesh, Demo::TestColors::cube_FaceColors); 
        
        
        
        // Cube version 2: placed behind the other cubes.  Testing textures 
        glm::mat4 cubeMatrix_v2 = AcornMath::BuildModelMatrix(
            basicShapes.cube_V2.transform.position,
            basicShapes.cube_V2.transform.scale,
            basicShapes.cube_V2.transform.rotationDegrees + basicShapes.cubeAuto.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection , basicShapes.cube_V2, cubeMatrix_v2, 
            testTexture_crate, true,	1.0f,1.0f
        ); 
        
        
        
        // Sphere: placed on the right.
        glm::mat4 sphereMatrix_1 = AcornMath::BuildModelMatrix(
            basicShapes.sphere_1.transform.position + basicShapes.sphereAuto.translation,
            basicShapes.sphere_1.transform.scale,
            basicShapes.sphere_1.transform.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, basicShapes.sphere_1, sphereMatrix_1,
            basicShapes.sphere_1.textureID, false, 1.0f, 0.0f
        );
        
        glm::mat4 sphereMatrix_2 = AcornMath::BuildModelMatrix(
            basicShapes.sphere_2.transform.position - basicShapes.sphereAuto.translation,
            basicShapes.sphere_2.transform.scale,
            basicShapes.sphere_2.transform.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, basicShapes.sphere_2, sphereMatrix_2, 
            basicShapes.sphere_2.textureID, false, 1.0f, 0.0f
        );
        
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
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::ModelInstance;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        const char* backpackPath = "models/backpack/backpack.obj";
        const char* backpackTexturePath = "models/backpack/diffuse.jpg";
        
        // Left Backpack, using assimp loader
        GpuModel backpackGpu_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(backpackPath, true, false); 
        // right Backpack, using my loader
        GpuModel backpackGpu_mine = OBJ_Loader::UploadModel(backpackPath, true, false);
        
        
        // Make instances of the models 
        ModelInstance backpack_assimp{&backpackGpu_assimp};
        ModelInstance backpack_mine{&backpackGpu_mine};
        
        GLuint sharedTexture = 0;
        
        Testing_TwoBackpacks()
        {
            
            stbi_set_flip_vertically_on_load(true);
            sharedTexture  = Texture::LoadTexture2D(backpackTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            backpack_assimp.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
            backpack_assimp.overrideColor = true;
            backpack_assimp.solidColor = glm::vec3(0.75f);
            backpack_assimp.textureID = sharedTexture ;
            
            
            backpack_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            backpack_mine.overrideColor = true;
            backpack_mine.solidColor = glm::vec3(0.75f);
            backpack_mine.textureID = sharedTexture ;
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
        Rendering::DrawModel(shaderProgram, view, projection, models.backpack_assimp, backpackMatrix_Left, models.backpack_assimp.textureID, true);
        
        
        const glm::mat4 backpackMatrix_Right = AcornMath::BuildModelMatrix(
            models.backpack_mine.transform.position + models.runtime_Info.translation,
            models.backpack_mine.transform.scale * models.runtime_Info.scale,
            models.backpack_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.backpack_mine, backpackMatrix_Right, models.backpack_mine.textureID, true);
    }
    
    
    
    struct Testing_TwoTeapots
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::ModelInstance;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        const char* teapotPath = "models/teapot/teapot.obj";
        const char* teapotTexturePath = "models/teapot/default.png";
        
        // Left Backpack, using assimp loader
        GpuModel teapotGpu_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(teapotPath, true, false); 
        // right Backpack, using my loader
        GpuModel teapotGpu_mine = OBJ_Loader::UploadModel(teapotPath, true, true, false);
        
        
        // Make instances of the models 
        ModelInstance teapot_assimp{&teapotGpu_assimp}; 
        ModelInstance teapot_mine{&teapotGpu_mine}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoTeapots()
        {
            stbi_set_flip_vertically_on_load(true);
            sharedTexture = Texture::LoadTexture2D(teapotTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            teapot_assimp.transform.position = glm::vec3(-3.0f, 0.0f, 0.0f);
            teapot_assimp.transform.scale = glm::vec3(0.025f);
            teapot_assimp.overrideColor = true;
            teapot_assimp.solidColor = glm::vec3(0.75f);
            teapot_assimp.textureID = sharedTexture;
            
            
            teapot_mine.transform.position = glm::vec3(3.0f, 0.0f, 0.0f);
            teapot_mine.transform.scale = glm::vec3(0.025f);
            teapot_mine.overrideColor = true;
            teapot_mine.solidColor = glm::vec3(0.75f);
            teapot_mine.textureID = sharedTexture;
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
        Rendering::DrawModel(shaderProgram, view, projection, models.teapot_assimp, teapotMatrix_Left, models.teapot_assimp.textureID, true);
        
        
        const glm::mat4 teapotMatrix_Right = AcornMath::BuildModelMatrix(
            models.teapot_mine.transform.position + models.runtime_Info.translation,
            models.teapot_mine.transform.scale * models.runtime_Info.scale,
            models.teapot_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.teapot_mine, teapotMatrix_Right, models.teapot_mine.textureID, true);
    }
    
    
    
    
    struct Testing_TwoIpshead
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::ModelInstance;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        const char* lpsheadPath = "models/lpshead/head.obj";               
        const char* lpsheadTexturePath = "models/lpshead/rendered.jpg";
        //const char* lpsheadTexturePath = "models/lpshead/lambertian.jpg";
        
        // Left Backpack, using assimp loader
        GpuModel lpsheadGpu_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(lpsheadPath, true, false); 
        // right Backpack, using my loader
        GpuModel lpsheadGpu_mine = OBJ_Loader::UploadModel(lpsheadPath, true, true, false);
        
        
        // Make instances of the models 
        ModelInstance lpshead_assimp{&lpsheadGpu_assimp}; 
        ModelInstance lpshead_mine{&lpsheadGpu_mine}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoIpshead()
        {
            stbi_set_flip_vertically_on_load(true);
            sharedTexture = Texture::LoadTexture2D(lpsheadTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            lpshead_assimp.transform.position = glm::vec3(-2.0f, 2.0f, 0.0f);
            lpshead_assimp.transform.scale = glm::vec3(10.0f);
            lpshead_assimp.overrideColor = true;
            lpshead_assimp.solidColor = glm::vec3(0.75f);
            lpshead_assimp.textureID = sharedTexture;
            
            
            lpshead_mine.transform.position = glm::vec3(2.0f, 2.0f, 0.0f);
            lpshead_mine.transform.scale = glm::vec3(10.0f);
            lpshead_mine.overrideColor = true;
            lpshead_mine.solidColor = glm::vec3(0.75f);
            lpshead_mine.textureID = sharedTexture;
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
        Rendering::DrawModel(shaderProgram, view, projection, models.lpshead_assimp, lpsheadMatrix_Left, models.lpshead_assimp.textureID, true);
        
        
        const glm::mat4 lpsheadMatrix_Right = AcornMath::BuildModelMatrix(
            models.lpshead_mine.transform.position + models.runtime_Info.translation,
            models.lpshead_mine.transform.scale * models.runtime_Info.scale,
            models.lpshead_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.lpshead_mine, lpsheadMatrix_Right, models.lpshead_mine.textureID, true);
    }
    
    
    
    
    struct Testing_TwoCubes
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::ModelInstance;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        const char* cubePath = "models/cube/cube.obj";
        const char* cubeTexturePath = "models/cube/default.png";
        
        // Left Backpack, using assimp loader
        GpuModel cubeGpu_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(cubePath, true, false); 
        // right Backpack, using my loader
        GpuModel cubeGpu_mine = OBJ_Loader::UploadModel(cubePath, true, false);
        
        
        // Make instances of the models 
        ModelInstance cube_assimp{&cubeGpu_assimp}; 
        ModelInstance cube_mine{&cubeGpu_mine}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoCubes()
        {
            stbi_set_flip_vertically_on_load(true);
            sharedTexture = Texture::LoadTexture2D(cubeTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            cube_assimp.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
            cube_assimp.transform.scale = glm::vec3(2.0f);
            cube_assimp.overrideColor = true;
            cube_assimp.solidColor = glm::vec3(0.75f);
            cube_assimp.textureID = sharedTexture;
            
            
            cube_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            cube_mine.transform.scale = glm::vec3(2.0f);
            cube_mine.overrideColor = true;
            cube_mine.solidColor = glm::vec3(0.75f);
            cube_mine.textureID = sharedTexture;
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
        Rendering::DrawModel(shaderProgram, view, projection, models.cube_assimp, cubeMatrix_Left, models.cube_assimp.textureID, true);
        
        
        const glm::mat4 cubeMatrix_Right = AcornMath::BuildModelMatrix(
            models.cube_mine.transform.position + models.runtime_Info.translation,
            models.cube_mine.transform.scale * models.runtime_Info.scale,
            models.cube_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.cube_mine, cubeMatrix_Right, models.cube_mine.textureID, true);
    }
    
    
    
    
    struct Testing_TwoPines
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::ModelInstance;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        const char* pinePath = "models/pine/scrubPine.obj";
        const char* pineTexturePath = "models/pine/LushPine.png";
        
        // Left Backpack, using assimp loader
        GpuModel pineGpu_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(pinePath, true, false); 
        // right Backpack, using my loader
        GpuModel pineGpu_mine = OBJ_Loader::UploadModel(pinePath, true, false);
        
        
        // Make instances of the models 
        ModelInstance pine_assimp{&pineGpu_assimp}; 
        ModelInstance pine_mine{&pineGpu_mine}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoPines()
        {
            stbi_set_flip_vertically_on_load(true);
            sharedTexture = Texture::LoadTexture2D(pineTexturePath);
            stbi_set_flip_vertically_on_load(false);
            
            pine_assimp.transform.position = glm::vec3(-3.0f, -2.0f, 0.5f);
            pine_assimp.transform.scale = glm::vec3(0.01f);
            pine_assimp.overrideColor = true;
            pine_assimp.solidColor = glm::vec3(0.75f);
            pine_assimp.textureID = sharedTexture;
            
            
            pine_mine.transform.position = glm::vec3(3.0f, -2.0f, 0.5f);
            pine_mine.transform.scale = glm::vec3(0.01f);
            pine_mine.overrideColor = true;
            pine_mine.solidColor = glm::vec3(0.75f);
            pine_mine.textureID = sharedTexture;
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
        Rendering::DrawModel(shaderProgram, view, projection, models.pine_assimp, pineMatrix_Left, models.pine_assimp.textureID, true);
        
        
        const glm::mat4 pineMatrix_Right = AcornMath::BuildModelMatrix(
            models.pine_mine.transform.position + models.runtime_Info.translation,
            models.pine_mine.transform.scale * models.runtime_Info.scale,
            models.pine_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.pine_mine, pineMatrix_Right, models.pine_mine.textureID, true);
    }
    
    
    
    
    struct Testing_TwoSportCars
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::ModelInstance;
        
        // controlled by keyboard toggles.
        GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        GeometryRuntimeValues_Transforms runtime_Info{};
        
        
        const char* sportCarsPath = "models/sportsCar/sportsCar.obj";
        // no texture
        
        // Left Backpack, using assimp loader
        GpuModel sportCarGpu_assimp = OBJ_Loader_Assimp::UploadModel_Assimp(sportCarsPath, true, false); 
        // right Backpack, using my loader
        GpuModel sportCarGpu_mine = OBJ_Loader::UploadModel(sportCarsPath, true, true, false);
        
        
        // Make instances of the models 
        ModelInstance sportCar_assimp{&sportCarGpu_assimp}; 
        ModelInstance sportCar_mine{&sportCarGpu_mine}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoSportCars()
        {
            //stbi_set_flip_vertically_on_load(true);
            //sharedTexture = Texture::LoadTexture2D(sportCarTexturePath);
            //stbi_set_flip_vertically_on_load(false);
            
            sportCar_assimp.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
            sportCar_assimp.overrideColor = true;
            sportCar_assimp.solidColor = glm::vec3(0.75f);
            //sportCar_assimp.textureID = sharedTexture;
            
            
            sportCar_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            sportCar_mine.overrideColor = true;
            sportCar_mine.solidColor = glm::vec3(0.75f);
            //sportCar_mine.textureID = sharedTexture;
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
        Rendering::DrawModel(shaderProgram, view, projection, models.sportCar_assimp, sportCarMatrix_Left, models.sportCar_assimp.textureID, true);
        
        
        const glm::mat4 sportCarMatrix_Right = AcornMath::BuildModelMatrix(
            models.sportCar_mine.transform.position + models.runtime_Info.translation,
            models.sportCar_mine.transform.scale * models.runtime_Info.scale,
            models.sportCar_mine.transform.rotationDegrees + models.runtime_Info.rotationDegrees
        );
        Rendering::DrawModel(shaderProgram, view, projection, models.sportCar_mine, sportCarMatrix_Right, models.sportCar_mine.textureID, true);
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
    using AcornData::CpuMeshData;
    using AcornData::GpuMesh;
    using AcornData::ModelInstance; 
    
    
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
    
    
    
    // Keep GPU-owning demo objects inside this scope.
    // Their destructors must run while the OpenGL context still exists.
    { 
        
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
        
        int selectedTest = static_cast<int>(Demo::TestScene::DefaultShapes);
        
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
        Rendering::DestroyGpuModel(basicShapes.pyramidGpu);
        Rendering::DestroyGpuModel(basicShapes.cubeGpu);
        Rendering::DestroyGpuModel(basicShapes.sphereGpu);
        
        Rendering::DestroyGpuModel(basicShapes.pyramidGp_v2);
        Rendering::DestroyGpuModel(basicShapes.cubeGpu_v2);
        
        
        // Model cleanup
        //Rendering::DestroyGpuModel(backpack);    
        Rendering::DestroyGpuModel(twoBackpacks.backpackGpu_assimp);    
        Rendering::DestroyGpuModel(twoBackpacks.backpackGpu_mine);  
        
        Rendering::DestroyGpuModel(twoTeapots.teapotGpu_assimp);    
        Rendering::DestroyGpuModel(twoTeapots.teapotGpu_mine);    
        
        Rendering::DestroyGpuModel(twoIpshead.lpsheadGpu_assimp);    
        Rendering::DestroyGpuModel(twoIpshead.lpsheadGpu_mine);    
        
        Rendering::DestroyGpuModel(twoCubes.cubeGpu_assimp);    
        Rendering::DestroyGpuModel(twoCubes.cubeGpu_mine);    
        
        Rendering::DestroyGpuModel(twoPines.pineGpu_assimp);    
        Rendering::DestroyGpuModel(twoPines.pineGpu_mine);    
        
        Rendering::DestroyGpuModel(twoSportCars.sportCarGpu_assimp);    
        Rendering::DestroyGpuModel(twoSportCars.sportCarGpu_mine);    
        
        
        glDeleteProgram(shaderProgram.ID);
        
    }  // GPU model/mesh destructors should run here as they go out of scope, before context shutdown.
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



// Project headers
#include "AcornMath.h"
#include "geometry/CpuMeshData.h"
#include "rendering/Camera.h"
#include "rendering/OpenGLErrors.h"
#include "rendering/GpuModel.h"
#include "rendering/Renderer.h"
#include "rendering/ShaderProgram.h" 
#include "SceneManager.h"
#include "testing/CarouselDemo.h"
#include "testing/Demo.h"

// Standard library
#include <array>
#include <cassert>      // assert 
#include <chrono>       // For measuring application load time 
#include <cmath>        // Scalar math for geometry and animation
#include <filesystem>
#include <iostream>     // std::cout and std::cerr for diagnostics
#include <initializer_list>
#include <optional>
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
    constexpr float FOV = 70.0f;
    
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
        
    
        return window; 
    }
    
    
    void processInput_ESCAPE(GLFWwindow* window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    
    struct RenderInputHistory
    {
        bool previousTab = false;
        bool previousCaps = false;
        bool previousEnter = false;
    }; 
    
    void processInput_RenderSettings(GLFWwindow* window, RenderInputHistory& history, Rendering::RenderSettings& settings, Rendering::CameraData& camera)
    {
        const bool tab = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tab && !history.previousTab)
        {
            settings.wireframe = !settings.wireframe;
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        // Set drawing mode to wireframe
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);        // Set drawing mode to filled, the default. 
        }
        history.previousTab = tab;
        
        const bool caps = glfwGetKey(window, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS;
        if (caps && !history.previousCaps)
        {
            settings.cullFaces = !settings.cullFaces;
        }
        history.previousCaps = caps;
        
        const bool enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enter && !history.previousEnter)
        {
            auto& type = camera.projectionSettings.projectionType;

            type = type == Rendering::ProjectionType::PERSPECTIVE
                ? Rendering::ProjectionType::ORTHOGRAPHIC
                : Rendering::ProjectionType::PERSPECTIVE;
        }
        history.previousEnter = enter;
        
    }
    
    
    
    struct SceneSelectionInputHistory
    {
        std::array<bool, 3> previousDown{};
    };

    // Returns an array index only on a new key press.
    // If several keys are newly pressed together, the lowest index wins.
    std::optional<std::size_t> ReadSceneSelection(
        GLFWwindow* window,
        SceneSelectionInputHistory& history)
    {
        constexpr std::array<int, 3> keys{
            GLFW_KEY_1,
            GLFW_KEY_2,
            GLFW_KEY_3
        };

        std::optional<std::size_t> requested;

        for (std::size_t i = 0; i < keys.size(); ++i)
        {
            const bool down =
                glfwGetKey(window, keys[i]) == GLFW_PRESS;

            if (down && !history.previousDown[i] &&
                !requested.has_value())
            {
                requested = i;
            }

            // Update every key, even after choosing a request.
            history.previousDown[i] = down;
        }

        return requested;
    }
    
}







namespace Demo
{   
    // move to scene? 
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
    
    
    
}






struct FrameClock
{
    double previousTime = 0.0;
    float deltaTime = 0.0f;

    void Reset(double now)
    {
        previousTime = now;
        deltaTime = 0.0f;
    }

    void Tick(double now)
    {
        deltaTime = static_cast<float>(now - previousTime);
        previousTime = now;
    }
};





int main(int argc, char **argv) {
    
    using AcornData::Transform;
    using AcornData::Vertex;
    using AcornData::CpuMeshData;
    using AcornData::RenderModelInstance; 
    
    
    // Temp program time measurement 
    const auto startupStart = std::chrono::steady_clock::now();

    std::cerr << "[Startup] Entered main\n";
    
    
    // ---- Setup Window ---- 
    
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
    
    // ---- Setup Shader Program ---- 
    
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
    
    
    
    
    // Keeping GPU-owning demo objects inside this scope.
    // Their destructors must run while the OpenGL context still exists.
    { 
        
        
        // ---- Loading Assets  ---- 
        
        // Temp program time measurement 
        const auto assetsStart = std::chrono::steady_clock::now();
        
        
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
        
        
        GLuint backpackTexture = Texture::LoadTexture2D("models/backpack/diffuse.jpg");
        if (backpackTexture == 0)
        {
            std::cerr << "[Warning] Failed to load texture: models/backpack/diffuse.jpg\n";
        }
        
        GLuint teapotTexture = Texture::LoadTexture2D("models/teapot/default.png");
        if (teapotTexture == 0)
        {
            std::cerr << "[Warning] Failed to load texture: models/teapot/default.png\n";
        }
        
        GLuint ipsheadTexture = Texture::LoadTexture2D("models/lpshead/rendered.jpg");
        if (ipsheadTexture == 0)
        {
            std::cerr << "[Warning] Failed to load texture: models/lpshead/rendered.jpg\n";
        }
        
        GLuint cubeTexture = Texture::LoadTexture2D("models/cube/default.png");
        if (cubeTexture == 0)
        {
            std::cerr << "[Warning] Failed to load texture: models/cube/default.png\n";
        }
        
        GLuint pineTexture = Texture::LoadTexture2D("models/pine/LushPine.png");
        if (pineTexture == 0)
        {
            std::cerr << "[Warning] Failed to load texture: models/pine/LushPine.png\n";
        }
        
        
        stbi_set_flip_vertically_on_load(false);
        
        
        // Owns this level's cached geometry and newly created scene objects.
        Scene activeScene{
            std::filesystem::current_path()
        };
        
        
        /*
        
        
        // --- testing new import pipeline --- 
        Demo::PopulateComparison_Backpack(activeScene, backpackTexture);
        //Demo::PopulateComparison_Teapot(activeScene, teapotTexture);
        //Demo::PopulateComparison_Ipshead(activeScene, ipsheadTexture);
        //Demo::PopulateComparison_Cube(activeScene, cubeTexture);
        Demo::PopulateComparison_Pine(activeScene, pineTexture);
        //Demo::PopulateComparison_SportsCar(activeScene, 0);
        
        */
        
        
        
        // ---- Scene Settings ---- 
        
        // Later test some active scenes with different populations, keypresses to switch scene 
        
        
        Scene& scene = activeScene;
        //scene.sceneID = 0;
        //scene.settings.splitScreen = AcornData::SplitScreenSettings::HorizontalSplit;
        
        // face culling    
        Rendering::RenderSettings renderSettings {};
        Rendering::ApplyRenderSettings(renderSettings); 
        
        RenderInputHistory renderSettingInputHistory = {};    
        
        // enable depth view 
        glEnable(GL_DEPTH_TEST);
        
        
        
        /*
        
        // ---- Camera ---- 
        
        scene.cameras.resize(2);
        
        // Resizing or appending the vector can screw up saved references, if that were to 
        // happen the saved sceneCamera and secondCamera would need to be set again before use
        SceneCamera& sceneCamera = scene.cameras[0];
        SceneCamera& secondCamera = scene.cameras[1];
        //SceneCamera& thirdCamera = scene.cameras[2];
        //SceneCamera& fourthCamera = scene.cameras[3];
        
        // camera 1
        sceneCamera.camera.projectionSettings.projectionType = Rendering::ProjectionType::PERSPECTIVE; 
        sceneCamera.camera.projectionSettings.verticalFovDegrees = 70.0f;
        sceneCamera.camera.projectionSettings.orthographicHeight = 10.0f;
        sceneCamera.camera.projectionSettings.nearPlane = 0.1f;
        sceneCamera.camera.projectionSettings.farPlane = 100.0f;
        
        // camera 2,  set up with camera 1 
        secondCamera.camera.projectionSettings = sceneCamera.camera.projectionSettings;
        
        secondCamera.target = sceneCamera.target;
        secondCamera.startingOffset = sceneCamera.startingOffset;
        secondCamera.orbitAngleDegrees = 180.0f;
        secondCamera.backgroundColor = scene.settings.defaultBackgroundColor;
        
        // camera 3
        //thirdCamera.backgroundColor = Palette::pink;
        
        // camera 4 
        
        
        // update each camera transform to match starting values 
        for (SceneCamera& camera : scene.cameras)
        {
            Demo::SetCameraOrbitPose(
                camera.camera,
                camera.target,
                camera.orbitAngleDegrees,
                camera.startingOffset);
        }
        
        
        */
        
        
        const float cameraSpeed = 10.0f;
        
        
        
        const auto sceneDescriptions = Demo::MakeDemoScenes();

        if (!LoadSceneFromDescription(activeScene, sceneDescriptions[0]))
        {
            std::cerr << "[Scene] Failed to load initial scene\n";
        }
        
        
        
        
        // Temp program time measurement 
        const auto assetsEnd = std::chrono::steady_clock::now();

        std::cerr
            << "[Startup] Asset setup: "
            << std::chrono::duration<double>(assetsEnd - assetsStart).count()
            << " seconds\n";
        
        
        
        
        // ---- Runtime values ---- 
        
        FrameClock clock;
        clock.Reset(glfwGetTime());
        
        
        FramebufferSize framebuffer{};
        
        
        SceneSelectionInputHistory sceneSelectionHistory;
        
        
        // change this to toggle between scenes 
        //int selectedTest = static_cast<int>(Demo::TestSceneObjects::Backpacks);     // what models to show
        
        
        
        const GLint lightDirectionLocation = glGetUniformLocation(shaderProgram.ID, "ulightDirection");
        float sunAngleDegrees = 0.0f;
        
        
        bool useBlackBackground = true;
        bool previousZeroDown = false;
        
        
        
        //  Testing weird camera layouts 
        const bool useCarouselExperiment = false;

        Demo::CarouselSettings carousel;

        carousel.layout =
            Demo::SplitScreenSettings_Extra_Carousel::CarouselHorizontalSplit;

        carousel.secondsPerLoop = 8.0f;
        
        
        
        
        
        // Temp program time measurement 
        std::cerr
            << "[Startup] Ready for loop after "
            << std::chrono::duration<double>(
                   std::chrono::steady_clock::now() - startupStart).count()
            << " seconds\n";
        
        
        
        
        // render loop
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            processInput_ESCAPE(window);
            
            // get time and deltaTime 
            clock.Tick(glfwGetTime());
            float deltaTime = clock.deltaTime;
            
            
            
            const auto requestedScene =
            ReadSceneSelection(window, sceneSelectionHistory);
            
            if (requestedScene.has_value())
            {
                const std::size_t index = *requestedScene;

                if (index < sceneDescriptions.size())
                {
                    
                    const bool loaded = LoadSceneFromDescription(
                        activeScene,
                        sceneDescriptions[index]);

                    if (loaded)
                    {
                        glfwSetWindowTitle(window, activeScene.name.c_str()); 
                        
                        std::cout
                            << "[Scene] Loaded "
                            << activeScene.name << '\n';
                    }
                    else
                    {
                        std::cerr
                            << "[Scene] Failed to load "
                            << sceneDescriptions[index].name << '\n';
                    }

                    // Synchronous loading must not become a movement timestep.
                    clock.Reset(glfwGetTime());
                    deltaTime = 0.0f;
                }
            }

            
            // Acquire camera access only after any scene replacement.
            // Failed loading deliberately leaves an empty scene.
            if (!scene.cameras.empty())
            {
                processInput_RenderSettings(
                    window,
                    renderSettingInputHistory,
                    renderSettings,
                    scene.cameras[0].camera);
                
                
                
                // switch between bright and dark background with key press 0 
                const bool zeroDown = glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS;
                
                if (zeroDown && !previousZeroDown)
                {
                    useBlackBackground = !useBlackBackground;
                }
                
                previousZeroDown = zeroDown;
                
                // background color for  camera 1
                scene.cameras[0].backgroundColor = useBlackBackground
                    ? glm::vec3(0.0f)
                    : glm::vec3(0.2f, 0.3f, 0.3f);
                
            }

            
            scene.UpdateScene(deltaTime);
            
            
            glfwGetFramebufferSize(
                window,
                &framebuffer.width,
                &framebuffer.height);
            
            
            // zero check skips drawing if there is no drawable area
            if (framebuffer.width == 0 || framebuffer.height == 0)
            {
                continue;
            }
            
            scene.ConfigureSceneViewports(framebuffer);
            
            
            if (useCarouselExperiment)
            {
                Demo::UpdateCarousel(carousel, deltaTime);
            }
            
            
            
            /* Orbiting camera code 
            for (SceneCamera& camera : scene.cameras)
            {
                if (camera.enabled)
                {
                    Demo::UpdateOrbitingCamera(camera, deltaTime);
                    
                    //Demo::UpdateCameraPos(window, camera, cameraSpeed, deltaTime);
                }
            }
            */
            
            
            //const glm::vec3 spinningSunDirection = Demo::SpinTheSun(deltaTime, sunAngleDegrees);
            //glUniform3fv(lightDirectionLocation, 1, glm::value_ptr(spinningSunDirection));
            
            
            
            Rendering::ApplyRenderSettings(renderSettings);

            // change so one scene uses this later 
            if (useCarouselExperiment)
            {
                Demo::DrawCarouselScene(
                    scene,
                    shaderProgram,
                    framebuffer,
                    carousel);
            }
            else
            {
                scene.DrawScene(shaderProgram);
            }
            
            glfwSwapBuffers(window);
        }
        
        // Texture cleanup 
        glDeleteTextures(1, &testTexture_crate);
        glDeleteTextures(1, &testTexture_face);

        glDeleteTextures(1, &backpackTexture);
        glDeleteTextures(1, &teapotTexture);
        glDeleteTextures(1, &ipsheadTexture);
        glDeleteTextures(1, &cubeTexture);
        glDeleteTextures(1, &pineTexture);
        
       
        
        
        glDeleteProgram(shaderProgram.ID);
        
    }  // GPU model/mesh destructors should run here as they go out of scope, before context shutdown.
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



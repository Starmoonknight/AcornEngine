// Project headers
#include "assets/SceneFile.h"
#include "rendering/Camera.h"
#include "rendering/OpenGLErrors.h"
#include "rendering/Renderer.h"
#include "rendering/ShaderProgram.h" 
#include "SceneManager.h"
#include "testing/CarouselDemo.h"
#include "testing/Demo.h"
#include "Utility/InputSystem.h"
#include "Utility/InputContext.h"


// Standard library
#include <array>
#include <cassert>      // assert 
#include <chrono>       // For measuring application load time 
#include <cmath>        // Scalar math for geometry and animation
#include <filesystem>
#include <iostream>     // std::cout and std::cerr for diagnostics
#include <initializer_list>
#include <unordered_map>
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
    constexpr int WindowWidth = 1200;
    constexpr int WindowHeight = 1000;
    
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
    
    
    
    // Shader initialization has succeeded above this point.
    // Keeping GPU-owning demo objects inside this scope.
    // Their destructors must run while the OpenGL context still exists.
    { 
        
        
        // ---- Setup Input ---- 
    
        // Destroyed when this scope ends, before glfwDestroyWindow().
        Input::InputSystem input(window);
    
        
        
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
        
        GLuint cobblestoneTexture = Texture::LoadTexture2D("textures/cobblestone_floor_07_diff_4k.jpg");
        if (cobblestoneTexture == 0)
        {
            std::cerr << "[Warning] Failed to load texture: textures/cobblestone_floor_07_diff_4k.jpg\n";
        }
        
        
        
        std::string playerFirstBody = "models/cube/cube.obj";
        std::string playerBody = "models/suzanne/suzanne.obj";
        std::string playerOtherBody = "models/lpshead/head.obj";
        
        
        // for saving textures path to a scene 
        const std::unordered_map<std::string, GLuint> sceneTextures{
            {"models/cube/default.png", cubeTexture},
            {"models/pine/LushPine.png", pineTexture},
            {"models/teapot/default.png", teapotTexture},
            {"models/backpack/diffuse.jpg", backpackTexture},
            {"textures/cobblestone_floor_07_diff_4k.jpg", cobblestoneTexture}
        };
        
        
        stbi_set_flip_vertically_on_load(false);
        
        
        // Owns this level's cached geometry and newly created scene objects.
        Scene activeScene{
            std::filesystem::current_path()
        };
        
                
        
        
        // ---- Scene Settings ---- 
        
        Scene& scene = activeScene;
        
        // face culling    
        Rendering::RenderSettings renderSettings {};
        Rendering::ApplyRenderSettings(renderSettings); 
        
        
        // enable depth view 
        glEnable(GL_DEPTH_TEST);
        
        
        
        
        
        
        /* // TEMPORARY EXPORT: run once, then remove this block.
        // Temporary code to make scenes and save thema s files
        const std::array<std::filesystem::path, 6> scenePaths{
            "scenes/basic_01.acorn",
            "scenes/basic_02.acorn",
            "scenes/basic_03.acorn",
            "scenes/autumn_forest.acorn",
            "scenes/teapot_space.acorn",
            "scenes/basalt_gallery.acorn"
        };
        
        const auto oldScenes = Demo::MakeDemoScenes();
        
        const std::array<AcornData::SceneDescription, 6> sceneDescriptions{
            oldScenes[0], 
            oldScenes[1], 
            oldScenes[2], 
            Demo::ForestScene::Build(),
            Demo::TeapotScene::Build(), 
            Demo::PillarScene::Build()
        };
        */ 
        
        
        
        
        // current scene data file paths
        const std::array<std::filesystem::path, 6> scenePaths{
            "scenes/basic_01.acorn",
            "scenes/basic_02.acorn",
            "scenes/basic_03.acorn",
            "scenes/autumn_forest.acorn",
            "scenes/teapot_space.acorn",
            "scenes/basalt_gallery.acorn"
        };
        
        // Filled from saved files when a scene is selected.
        // Keep the descriptions so F5 can write the loaded starting data.
        std::array<AcornData::SceneDescription, 6> sceneDescriptions{};
        
        
        
        // Lambda for scene switching showcase 
        std::size_t currentSceneIndex = 0;
        bool multiplayerEnabled = true;

        auto switchScene = [&](std::size_t index) -> bool
        {
            if (index >= scenePaths.size())
                return false;

            // Parse first. A malformed/missing file must not clear the
            // currently active runtime scene.
            std::string parseError;

            if (!SceneFile::Read(
                    scenePaths[index],
                    sceneDescriptions[index],
                    parseError))
            {
                std::cerr << "[Scene] " << parseError << '\n';
                return false;
            }

            // Parsing succeeded: now replace the runtime scene.
            bool loaded = LoadSceneFromDescription(
                activeScene, sceneDescriptions[index]);

            if (loaded)
            {
                if (index <= 2)
                {
                    loaded = Demo::CreatePlayers(activeScene, cubeTexture, playerFirstBody);
                }
                else if (index == 3)
                {
                    loaded = Demo::CreatePlayers(activeScene, cubeTexture, playerOtherBody);
                }
                else
                {
                    loaded = Demo::CreatePlayers(activeScene, cubeTexture, playerBody);
                }
            }
            
            if (!loaded)
            {
                activeScene.Clear();
                std::cerr << "[Scene] Failed to load scene " << index << '\n';
                return false;
            }

            currentSceneIndex = index;

            // when multiplayer is disabled, deactivate the second player parts 
            activeScene.cameras[1].enabled = multiplayerEnabled;
            activeScene.players[1].body.renderSettings.visible =
                multiplayerEnabled;
            
            
            
            // Temporary demo texture assignment.
            // The texture is owned by main and remains alive across scene switches.
            const auto& descriptions = sceneDescriptions[index].objects;

            // LoadSceneFromDescription adds placed objects in description order.
            // Player bodies are separate and do not enter placedObjects.
            for (std::size_t i = 0; i < descriptions.size(); ++i)
            {
                const std::string& path = descriptions[i].textureFilePath;
                auto& instance = activeScene.placedObjects[i].instance;

                instance.textureID = 0;

                if (path.empty())
                    continue; // Preserve textureExpected for deliberate error tests.

                const auto found = sceneTextures.find(path);

                if (found != sceneTextures.end())
                {
                    instance.textureID = found->second;
                }
                else
                {
                    std::cerr << "[Texture] No loaded texture for: " << path << '\n';
                }
            }            
            

            glfwSetWindowTitle(window, activeScene.name.c_str());
            return true;
        };
        
        
        
        // using the lambda to load start scene
        // Paths such as "scenes/basic_01.acorn" are relative to this directory.
        std::cout << "[Startup] Working directory: "
                  << std::filesystem::current_path().string()
                  << '\n';

        if (!switchScene(0))
        {
            std::cerr
                << "[Startup] Initial scene failed to load. "
                << "Use keys 1-6 to retry or select another scene.\n";
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
        
        
        // change this to toggle between scenes 
        //int selectedTest = static_cast<int>(Demo::TestSceneObjects::Backpacks);     // what models to show
        
        
        
        const GLint lightDirectionLocation = glGetUniformLocation(shaderProgram.ID, "ulightDirection");
        float sunAngleDegrees = 0.0f;
        
        
        bool useBlackBackground = false;
        
        
        
        
        
        // ----- Testing Zone ----- 
        
        //  Testing weird camera layouts 
        bool useCarouselExperiment = false;

        Demo::CarouselSettings carousel;

        carousel.layout =
            Demo::ESplitScreenSettings_Extra_Carousel::CarouselHorizontalSplit;

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
            
            
            // --------- Collect Inputs --------- 
            
            input.Update();
            const Demo::FrameInput actions = Demo::ReadFrameInput(input);

            
            // --------- Apply Inputs --------- 
            
            if (actions.quit)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            if (actions.toggleWireframe)
            {
                renderSettings.wireframe = !renderSettings.wireframe;
            }

            if (actions.toggleCulling)
            {
                renderSettings.cullFaces = !renderSettings.cullFaces;
            }

            if (actions.toggleBackground)
            {
                useBlackBackground = !useBlackBackground;                
            }
            
            if (actions.toggleMultiplayer)
            {
                multiplayerEnabled = !multiplayerEnabled;
            }
            
            if (actions.toggleCarousel)
            {
                useCarouselExperiment = !useCarouselExperiment;

                // Start each activation from the normal, unshifted arrangement.
                if (useCarouselExperiment)
                    carousel.phase = 0.0f;
            }
            
            
            // get time and deltaTime 
            clock.Tick(glfwGetTime());
            float deltaTime = clock.deltaTime;
            
            
            
            
            // --------- Handle manual scene change request --------- 
            
            // to avoid some behavior right after a scene switch             
            bool switchedThisFrame = false;
            
            if (actions.requestedScene.has_value())
            {
                switchScene(*actions.requestedScene);

                clock.Reset(glfwGetTime());
                deltaTime = 0.0f;
                switchedThisFrame = true;
            }
            
            
            // --------- Update scene logic --------- 
            
            // update scene logic 
            const SceneUpdateResult updateResult = scene.UpdateScene( 
                deltaTime,
                actions.players,
                multiplayerEnabled, 
                switchedThisFrame);

            
            
            // --------- Check for and handle Goal reached  --------- 
            
            if (updateResult.goalReached)
            {
                // Round-robin load scenes when goal is reached
                const std::size_t next =
                    (currentSceneIndex + 1) % sceneDescriptions.size();

                switchScene(next);

                clock.Reset(glfwGetTime());
                deltaTime = 0.0f;
                switchedThisFrame = true;
            }
            
            
            
            // --------- Temporary: Write current scene to file  --------- 
            
            if (actions.saveScene && scene.playersReady)
            {
                std::error_code directoryError;
                std::filesystem::create_directories("scenes", directoryError);

                if (directoryError)
                {
                    std::cerr << "[Save] " << directoryError.message() << '\n';
                }
                else
                {
                    std::string error;

                    if (SceneFile::Write(
                            scenePaths[currentSceneIndex],
                            sceneDescriptions[currentSceneIndex],
                            error))
                    {
                        std::cout << "[Save] "
                                  << scenePaths[currentSceneIndex].string() << '\n';
                    }
                    else
                    {
                        std::cerr << "[Save] " << error << '\n';
                    }
                }
            }
            
            
            
            
            
            
            // --------- Apply presentation settings --------- 
            
            // Acquire camera access only after any scene replacement.
            // Failed loading deliberately leaves an empty scene.
            if (actions.toggleProjection && !scene.cameras.empty())
            {
                auto& projection =
                    scene.cameras[0].camera.projectionSettings.projectionType;

                projection =
                    projection == Rendering::EProjectionType::PERSPECTIVE
                    ? Rendering::EProjectionType::ORTHOGRAPHIC
                    : Rendering::EProjectionType::PERSPECTIVE;
            }
            

            // background color for  camera 1
            if (!scene.cameras.empty())
            {
                scene.cameras[0].backgroundColor = useBlackBackground
                    ? glm::vec3(0.0f)
                    : scene.settings.defaultBackgroundColor;
                
                    //: glm::vec3(0.2f, 0.3f, 0.3f);
            }
            
            
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
            
            
            Rendering::ApplyRenderSettings(renderSettings);

            
            
            // --------- Draw using normal or carousel rendering --------- 
            
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
        glDeleteTextures(1, &cobblestoneTexture);
        
       
        
        
        glDeleteProgram(shaderProgram.ID);
        
    }  // GPU model/mesh destructors should run here as they go out of scope, before context shutdown.
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



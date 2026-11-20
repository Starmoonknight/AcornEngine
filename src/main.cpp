// Project headers
#include "AcornMath.h"
#include "geometry/CpuMeshData.h"
#include "rendering/Camera.h"
#include "rendering/OpenGLErrors.h"
#include "rendering/GpuModel.h"
#include "rendering/Renderer.h"
#include "rendering/ShaderProgram.h" 
#include "SceneManager.h"
#include "testing/Demo.h"



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
    
    
}







namespace Demo
{   
    
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
    
    
    
    glm::mat4 SpinTheCamera(float deltaTime, float& cameraAngleDegrees, const glm::vec3& target)
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
    
    
    // ---- Setup Window Framebuffer Callback ---- 
    
    // NOTE: This works well for single camera use but maybe not for splitscreen. 
    
    // will supply the correct framebuffer height automatically and width on window resize 
    // means that, whenever the framebuffer size changes, make rendering occupy its full area.
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    FramebufferSize framebuffer{};
    
    // set viewport once during setup, since registering the callback does not set it 
    glfwGetFramebufferSize(window, &framebuffer.width, &framebuffer.height);
    glViewport(0, 0, framebuffer.width, framebuffer.height);
    
    
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
        
        
        // preparing rendering objects, week2 shapes  
        Demo::Testing_DefaultShapes basicShapes {testTexture_crate}; 
        
        
        //  preparing rendering objects, week3 models 
        Demo::Testing_TwoBackpacks twoBackpacks {};
        
        Demo::Testing_TwoTeapots twoTeapots {};
        Demo::Testing_TwoIpshead twoIpshead {};
        Demo::Testing_TwoCubes twoCubes {};
        Demo::Testing_TwoPines twoPines {};
        Demo::Testing_TwoSportCars twoSportCars {};
        
        
        
        
        // ---- Scene Settings ---- 
        
        Scene scene;
        scene.SceneID = 0;
        
        // face culling    
        Rendering::RenderSettings renderSettings {};
        Rendering::ApplyRenderSettings(renderSettings); 
        
        RenderInputHistory renderSettingInputHistory = {};    
        
        // enable depth view 
        glEnable(GL_DEPTH_TEST);
        
        
        // Register all nine default-shape instances
        for (auto* model : {
            &basicShapes.basicPyramid_1,
            &basicShapes.basicPyramid_2,
            &basicShapes.pyramid_V1,
            &basicShapes.pyramid_manual,
            &basicShapes.basicCube_1,
            &basicShapes.basicCube_2,
            &basicShapes.cube_V2,
            &basicShapes.sphere_1,
            &basicShapes.sphere_2})
        {
            AddSceneModel(scene, model);
        }
        
        // and Register all the loaded test-model instances
        for (auto* model : {
            &twoBackpacks.backpack_assimp,
            &twoBackpacks.backpack_mine,
            &twoTeapots.teapot_assimp,
            &twoTeapots.teapot_mine,
            &twoIpshead.lpshead_assimp,
            &twoIpshead.lpshead_mine,
            &twoCubes.cube_assimp,
            &twoCubes.cube_mine,
            &twoPines.pine_assimp,
            &twoPines.pine_mine,
            &twoSportCars.sportCar_assimp,
            &twoSportCars.sportCar_mine})
        {
            model->renderSettings.lightingRequested = true;
            AddSceneModel(scene, model);
        }
        
        
        
        
        
        // ---- Camera ---- 
        
        
        scene.cameras.emplace_back();
        
        SceneCamera& sceneCamera = scene.cameras.front();
        sceneCamera.camera.projectionSettings.projectionType = Rendering::ProjectionType::PERSPECTIVE; 
        sceneCamera.camera.projectionSettings.verticalFovDegrees = 70.0f;
        sceneCamera.camera.projectionSettings.orthographicHeight = 10.0f;
        sceneCamera.camera.projectionSettings.nearPlane = 0.1f;
        sceneCamera.camera.projectionSettings.farPlane = 100.0f;
        
        Demo::SetCameraOrbitPose(
            sceneCamera.camera,
            sceneCamera.target,
            sceneCamera.orbitAngleDegrees,
            sceneCamera.startingOffset);
        
        
        // Testing camera 
        /*
        const glm::vec3 cameraTarget(0.0f);
        const glm::vec3 startingOffset(4.0f, 5.0f, 7.0f); 
        
        float cameraAngleDegrees = 0.0f;
        const float cameraSpeed = 0.05f;
        
        Rendering::CameraData cameraData(glm::vec3(4.0f, 5.0f, 7.0f));
        cameraData.projectionSettings.projectionType = Rendering::ProjectionType::PERSPECTIVE; 
        cameraData.projectionSettings.verticalFovDegrees = 70.0f;
        cameraData.projectionSettings.orthographicHeight = 10.0f;
        cameraData.projectionSettings.nearPlane = nearPlane;
        cameraData.projectionSettings.farPlane = farPlane;

        Demo::SetCameraOrbitPose(
            cameraData,
            cameraTarget,
            cameraAngleDegrees,
            startingOffset);
        
        glm::mat4 view;
        glm::mat4 projection;
        */
        
        
        
        
        
        
        
        
        // ---- Runtime values ---- 
        
        FrameClock clock;
        clock.Reset(glfwGetTime());
        
        
        int selectedTest = static_cast<int>(Demo::TestSceneObjects::DefaultShapes);     // what models to show
        
        
        
        const GLint lightDirectionLocation = glGetUniformLocation(shaderProgram.ID, "ulightDirection");
        float sunAngleDegrees = 0.0f;
        
        
        bool useBlackBackground = true;
        bool previousZeroDown = false;
        
        
        
        
        
        
        // render loop
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            processInput_ESCAPE(window);
            
            // get time and deltaTime 
            clock.Tick(glfwGetTime());
            float deltaTime = clock.deltaTime;
            
            
            // render settings and camera projection 
            processInput_RenderSettings(
                window, 
                renderSettingInputHistory, 
                renderSettings, 
                sceneCamera.camera);
            
            
            
            // switch between bright and dark background with key press 0 
            const bool zeroDown = glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS;
            
            if (zeroDown && !previousZeroDown)
            {
                useBlackBackground = !useBlackBackground;
            }
            
            previousZeroDown = zeroDown;
            
            // backgrround color for single camera 
            sceneCamera.backgroundColor = useBlackBackground
                ? glm::vec3(0.0f)
                : glm::vec3(0.2f, 0.3f, 0.3f);
            
            
            
            
            
            glfwGetFramebufferSize(
                window,
                &framebuffer.width,
                &framebuffer.height);
            
            // zero check skips drawing if there is no drawable area
            if (framebuffer.width == 0 || framebuffer.height == 0)
            {
                continue;
            }
            
            ConfigureSceneViewports(scene, framebuffer);
            Demo::UpdateOrbitingCamera(sceneCamera, deltaTime); 
            
            
            
            //view = Demo::SpinTheCamera(deltaTime, cameraAngleDegrees, cameraTarget);
            //Demo::UpdateCameraPos(window, cameraData, cameraSpeed);
            
            
            
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
            
            
            // hide all registered models 
            for (AcornData::RenderModelInstance* model : scene.models)
            {
                if (model != nullptr)
                    model->renderSettings.visible = false;
            }
            
            const Demo::TestSceneObjects selected = 
                static_cast<Demo::TestSceneObjects>(selectedTest);

            const bool showAllObjModels =
                selected == Demo::TestSceneObjects::AllModels;
            
            
            
            if (selected == Demo::TestSceneObjects::DefaultShapes)
            {
                for (auto* model : {
                    &basicShapes.basicPyramid_1,
                    &basicShapes.basicPyramid_2,
                    &basicShapes.pyramid_V1,
                    &basicShapes.pyramid_manual,
                    &basicShapes.basicCube_1,
                    &basicShapes.basicCube_2,
                    &basicShapes.cube_V2,
                    &basicShapes.sphere_1,
                    &basicShapes.sphere_2})
                {
                    model->renderSettings.visible = true;
                }

                Demo::Update_TestingDefaultShapes(window, basicShapes, deltaTime);
            }
            
            
            if (showAllObjModels || selected == Demo::TestSceneObjects::Backpacks)
            {
                twoBackpacks.backpack_assimp.renderSettings.visible = true;
                twoBackpacks.backpack_mine.renderSettings.visible = true;

                Demo::UpdateModelPair(
                    window,
                    twoBackpacks.runtime_Inputs,
                    twoBackpacks.runtime_Info,
                    twoBackpacks.backpack_assimp,
                    twoBackpacks.backpack_mine,
                    deltaTime);
            }

            if (showAllObjModels || selected == Demo::TestSceneObjects::Teapots)
            {
                twoTeapots.teapot_assimp.renderSettings.visible = true;
                twoTeapots.teapot_mine.renderSettings.visible = true;

                Demo::UpdateModelPair(
                    window,
                    twoTeapots.runtime_Inputs,
                    twoTeapots.runtime_Info,
                    twoTeapots.teapot_assimp,
                    twoTeapots.teapot_mine,
                    deltaTime);
            }

            if (showAllObjModels || selected == Demo::TestSceneObjects::Cubes)
            {
                twoCubes.cube_assimp.renderSettings.visible = true;
                twoCubes.cube_mine.renderSettings.visible = true;

                Demo::UpdateModelPair(
                    window,
                    twoCubes.runtime_Inputs,
                    twoCubes.runtime_Info,
                    twoCubes.cube_assimp,
                    twoCubes.cube_mine,
                    deltaTime);
            }

            if (showAllObjModels || selected == Demo::TestSceneObjects::Pines)
            {
                twoPines.pine_assimp.renderSettings.visible = true;
                twoPines.pine_mine.renderSettings.visible = true;

                Demo::UpdateModelPair(
                    window,
                    twoPines.runtime_Inputs,
                    twoPines.runtime_Info,
                    twoPines.pine_assimp,
                    twoPines.pine_mine,
                    deltaTime);
            }

            if (showAllObjModels || selected == Demo::TestSceneObjects::SportCars)
            {
                twoSportCars.sportCar_assimp.renderSettings.visible = true;
                twoSportCars.sportCar_mine.renderSettings.visible = true;

                Demo::UpdateModelPair(
                    window,
                    twoSportCars.runtime_Inputs,
                    twoSportCars.runtime_Info,
                    twoSportCars.sportCar_assimp,
                    twoSportCars.sportCar_mine,
                    deltaTime);
            }

            if (showAllObjModels || selected == Demo::TestSceneObjects::Ipshead)
            {
                twoIpshead.lpshead_assimp.renderSettings.visible = true;
                twoIpshead.lpshead_mine.renderSettings.visible = true;

                Demo::UpdateModelPair(
                    window,
                    twoIpshead.runtime_Inputs,
                    twoIpshead.runtime_Info,
                    twoIpshead.lpshead_assimp,
                    twoIpshead.lpshead_mine,
                    deltaTime);
            }
            
            
            
            
            
            Rendering::ApplyRenderSettings(renderSettings);
            DrawScene(scene, shaderProgram);
            
            glfwSwapBuffers(window);
        }
        
        // Texture cleanup 
        glDeleteTextures(1, &testTexture_crate);
        glDeleteTextures(1, &testTexture_face);

        glDeleteTextures(1, &twoBackpacks.sharedTexture);
        glDeleteTextures(1, &twoTeapots.sharedTexture);
        glDeleteTextures(1, &twoIpshead.sharedTexture);
        glDeleteTextures(1, &twoCubes.sharedTexture);
        glDeleteTextures(1, &twoPines.sharedTexture);
        glDeleteTextures(1, &twoSportCars.sharedTexture);
        
        
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



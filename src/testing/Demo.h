#pragma once


// Project headers
#include "AcornMath.h"
#include "assets/ModelUpload.h"
#include "geometry/BasicMeshes.h"
#include "rendering/Camera.h"
#include "rendering/GpuModel.h"
#include "rendering/Renderer.h"
#include "rendering/RenderInstances.h"
#include "Palette.h"


// Standard library


// Third-party libraries
#include <glfw/glfw3.h>
#include <glm/glm.hpp>     
#include "stb_image.h"


struct SceneCamera;

namespace Demo
{
    
    
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
    
    
    
    struct Testing_DefaultShapes 
    {
        using Transform = AcornData::Transform;
        using CpuMeshData = AcornData::CpuMeshData;
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::RenderModelInstance;
        
        
        // controlled by keyboard toggles.
        Demo::GeometryRuntimeValues_Inputs runtime_Inputs{}; 
        Demo::GeometryRuntimeValues_Transforms runtime_Info{};
        // Automatic animations.
        Demo::GeometryRuntimeValues_Transforms cubeAuto{};
        Demo::GeometryRuntimeValues_Transforms sphereAuto{};
        Demo::GeometryRuntimeValues_Transforms pyramidAuto{};
        
        Testing_DefaultShapes(GLuint testTexture_crate)
        {
            cubeAuto.bRotateY = true;
            sphereAuto.bTranslateZ = true;
            pyramidAuto.bScaleY = true;
            
            basicPyramid_2.renderSettings.overrideColor = true; 
            basicPyramid_2.renderSettings.tintColor = Palette::blue; 
            
            basicCube_2.renderSettings.overrideColor = true; 
            basicCube_2.renderSettings.tintColor = Palette::pink;
            
            sphere_2.renderSettings.overrideColor = true; 
            sphere_2.renderSettings.tintColor = Palette::forestGreen;
            
            pyramid_V1.renderSettings.lightingRequested = true; 
            pyramid_manual.renderSettings.lightingRequested = true; 
                        
            cube_V2.textureID = testTexture_crate;
            cube_V2.renderSettings.tintTextureAmount = 1.0f;
            cube_V2.renderSettings.lightingRequested = true; 
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
        ModelInstance basicPyramid_2{&pyramidGpu, Transform{glm::vec3(-4.0f, 0.0f, -2.5f), glm::vec3(180.0f, 0.0f, 0.0f)}}; 
        
        ModelInstance pyramid_V1{&pyramidGp_v2, Transform{glm::vec3(-4.0f, 0.0f, -6.0f)}};
        ModelInstance pyramid_manual{&pyramidGp_v2, Transform{glm::vec3(0.0f, 0.0f, 4.0f)}};
    
        
        ModelInstance basicCube_1{&cubeGpu, Transform{glm::vec3(0.0f, 0.0f, 1.0f)}};
        ModelInstance basicCube_2{&cubeGpu, Transform{glm::vec3(0.0f, 0.0f, -2.5f)}};
        
        ModelInstance cube_V2{&cubeGpu_v2, Transform{glm::vec3(0.0f, 0.0f, -7.0f)}};
        
        
        ModelInstance sphere_1{&sphereGpu, Transform{glm::vec3(3.0f, 0.0f, -1.0f)}};
        ModelInstance sphere_2{&sphereGpu, Transform{glm::vec3(4.5f, 0.0f, -2.5f)}}; 
    };
    
    
    
    struct Testing_TwoBackpacks
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::RenderModelInstance;
        
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
            backpack_assimp.renderSettings.overrideColor = true;
            backpack_assimp.renderSettings.tintColor = glm::vec3(0.75f);
            backpack_assimp.textureID = sharedTexture ;
            
            
            backpack_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            backpack_mine.renderSettings.overrideColor = true;
            backpack_mine.renderSettings.tintColor = glm::vec3(0.75f);
            backpack_mine.textureID = sharedTexture ;
        }
    };
    
    
    
    struct Testing_TwoTeapots
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::RenderModelInstance;
        
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
            teapot_assimp.renderSettings.overrideColor = true;
            teapot_assimp.renderSettings.tintColor = glm::vec3(0.75f);
            teapot_assimp.textureID = sharedTexture;
            
            
            teapot_mine.transform.position = glm::vec3(3.0f, 0.0f, 0.0f);
            teapot_mine.transform.scale = glm::vec3(0.025f);
            teapot_mine.renderSettings.overrideColor = true;
            teapot_mine.renderSettings.tintColor = glm::vec3(0.75f);
            teapot_mine.textureID = sharedTexture;
        }
    };
    
    
    
    struct Testing_TwoIpshead
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::RenderModelInstance;
        
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
            lpshead_assimp.renderSettings.overrideColor = true;
            lpshead_assimp.renderSettings.tintColor = glm::vec3(0.75f);
            lpshead_assimp.textureID = sharedTexture;
            
            
            lpshead_mine.transform.position = glm::vec3(2.0f, 2.0f, 0.0f);
            lpshead_mine.transform.scale = glm::vec3(10.0f);
            lpshead_mine.renderSettings.overrideColor = true;
            lpshead_mine.renderSettings.tintColor = glm::vec3(0.75f);
            lpshead_mine.textureID = sharedTexture;
        }
    };
    
    
    
    struct Testing_TwoCubes
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::RenderModelInstance;
        
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
            cube_assimp.renderSettings.overrideColor = true;
            cube_assimp.renderSettings.tintColor = glm::vec3(0.75f);
            cube_assimp.textureID = sharedTexture;
            
            
            cube_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            cube_mine.transform.scale = glm::vec3(2.0f);
            cube_mine.renderSettings.overrideColor = true;
            cube_mine.renderSettings.tintColor = glm::vec3(0.75f);
            cube_mine.textureID = sharedTexture;
        }
    };
    
    
    
    struct Testing_TwoPines
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::RenderModelInstance;
        
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
            pine_assimp.renderSettings.overrideColor = true;
            pine_assimp.renderSettings.tintColor = glm::vec3(0.75f);
            pine_assimp.textureID = sharedTexture;
            
            
            pine_mine.transform.position = glm::vec3(3.0f, -2.0f, 0.5f);
            pine_mine.transform.scale = glm::vec3(0.01f);
            pine_mine.renderSettings.overrideColor = true;
            pine_mine.renderSettings.tintColor = glm::vec3(0.75f);
            pine_mine.textureID = sharedTexture;
        }
    };
    
    
    
    struct Testing_TwoSportCars
    {
        using GpuModel = AcornData::GpuModel;
        using ModelInstance = AcornData::RenderModelInstance;
        
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
            sportCar_assimp.renderSettings.overrideColor = true;
            sportCar_assimp.renderSettings.tintColor = glm::vec3(0.75f);
            //sportCar_assimp.textureID = sharedTexture;
            
            
            sportCar_mine.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
            sportCar_mine.renderSettings.overrideColor = true;
            sportCar_mine.renderSettings.tintColor = glm::vec3(0.75f);
            //sportCar_mine.textureID = sharedTexture;
        }
    };
    
    
    
    
    enum class TestSceneObjects
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
    
    
    
    
    void HandleRuntimeInputs(
        GLFWwindow* window, 
        GeometryRuntimeValues_Inputs& inputs, 
        GeometryRuntimeValues_Transforms& values);
    
    
    
    void Update_Transforms(GeometryRuntimeValues_Transforms& values, float deltaTime);
    
    
    
    void Update_TestingDefaultShapes(
        GLFWwindow* window, 
        Testing_DefaultShapes& basicShapes, 
        float deltaTime);
    
    void Draw_TestingDefaultShapes(
        const ShaderProgram& shaderProgram, 
        Testing_DefaultShapes& basicShapes, 
        const Rendering::CameraMatrices& viewMatrices); 
    
    
    
    
    
    
    void UpdateModelPair(
        GLFWwindow* window,
        GeometryRuntimeValues_Inputs& inputs,
        GeometryRuntimeValues_Transforms& animation,
        AcornData::RenderModelInstance& left,
        AcornData::RenderModelInstance& right,
        float deltaTime); 
    
    
    
    // not in use, testing scene based drawing 
    void DrawAnimatedModel(
        const ShaderProgram& shader,
        const Rendering::CameraMatrices& viewMatrices,
        const AcornData::RenderModelInstance& model);

    // not in use, testing scene based drawing 
    void DrawModelPair(
        const ShaderProgram& shader,
        const Rendering::CameraMatrices& viewMatrices,
        const AcornData::RenderModelInstance& left,
        const AcornData::RenderModelInstance& right);
    
    
    
    
    
    void SetCameraOrbitPose(
        Rendering::CameraData& camera,
        const glm::vec3& target,
        float angleDegrees,
        glm::vec3 startingOffset);
    
    
    
    void UpdateOrbitingCamera(SceneCamera& camera, float deltaTime);
    
    
    void UpdateCameraPos(
        GLFWwindow* window, 
        Rendering::CameraData& cameraData, 
        float cameraSpeed, 
        float deltaTime);
    
    
    
}


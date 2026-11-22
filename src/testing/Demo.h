#pragma once


// Project headers
#include "AcornMath.h"
#include "assets/ModelCache.h"
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

namespace AcornData
{
    struct SceneDescription;
}

struct Scene;
struct SceneObject;
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
    
    
    
    
    void HandleRuntimeInputs(
        GLFWwindow* window, 
        GeometryRuntimeValues_Inputs& inputs, 
        GeometryRuntimeValues_Transforms& values);
    
    
    
    void Update_Transforms(GeometryRuntimeValues_Transforms& values, float deltaTime);
    
    
    
    
    
    
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
    
    
    
    
    
    
    std::array<AcornData::SceneDescription, 3> MakeDemoScenes(); 
    
    
    void ApplyDemoAppearance_Backpack(SceneObject& object, GLuint backpackTexture);
    void ApplyDemoAppearance_Teapot(SceneObject& object, GLuint teapotTexture);
    void ApplyDemoAppearance_Ipshead(SceneObject& object, GLuint ipsheadTexture);
    void ApplyDemoAppearance_Cube(SceneObject& object, GLuint cubeTexture);
    void ApplyDemoAppearance_Pine(SceneObject& object, GLuint pineTexture);
    
    
    // Adds the comparison objects to loaded.
    // Does not unload existing objects or own the resulting resources.
    void PopulateComparison_Backpack(
        Scene& loaded,
        GLuint textureID);
    
    void PopulateComparison_Teapot(
        Scene& loaded,
        GLuint textureID);
    
    
    void PopulateComparison_Ipshead(
        Scene& loaded,
        GLuint textureID);
    
    
    void PopulateComparison_Cube(
        Scene& loaded,
        GLuint textureID);
    
    
    void PopulateComparison_Pine(
        Scene& loaded,
        GLuint textureID);
    
    
    
    void PopulateComparison_SportsCar(
        Scene& loaded,
        GLuint textureID);
    
    
    
    
    
    
    
    
    
    
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
        const GpuModel* backpackGpu_assimp = nullptr;
        // right Backpack, using my loader
        const GpuModel* backpackGpu_mine = nullptr;
        
        
        // Make instances of the models 
        ModelInstance backpack_assimp{};
        ModelInstance backpack_mine{};
        
        GLuint sharedTexture = 0;
        
        Testing_TwoBackpacks(
            Assets::ModelCache& cache,
            bool verboseLogs = false)
        {
            Assets::ModelSource source;
            source.filePath = backpackPath;
            source.importSettings.flipUVs = true;

            // Keep the two importer results separate for comparison.
            source.importSettings.importer =
                Assets::ModelImporter::Assimp;

            backpackGpu_assimp = cache.GetOrLoad(source, verboseLogs);

            source.importSettings.importer =
                Assets::ModelImporter::CustomObj;
            source.importSettings.skipInvalidFaces = false;

            backpackGpu_mine = cache.GetOrLoad(source, verboseLogs);

            backpack_assimp.gpuModel = backpackGpu_assimp;
            backpack_mine.gpuModel = backpackGpu_mine; 
            
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
        const GpuModel* teapotGpu_assimp  = nullptr;
        // right Backpack, using my loader
        const GpuModel* teapotGpu_mine  = nullptr;
        
        
        // Make instances of the models 
        ModelInstance teapot_assimp{}; 
        ModelInstance teapot_mine{}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoTeapots(
            Assets::ModelCache& cache,
            bool verboseLogs = false)
        {
            Assets::ModelSource source;
            source.filePath = teapotPath;
            source.importSettings.flipUVs = true;

            // Keep the two importer results separate for comparison.
            source.importSettings.importer =
                Assets::ModelImporter::Assimp;

            teapotGpu_assimp = cache.GetOrLoad(source, verboseLogs);

            source.importSettings.importer =
                Assets::ModelImporter::CustomObj;
            source.importSettings.skipInvalidFaces = false;

            teapotGpu_mine = cache.GetOrLoad(source, verboseLogs);

            teapot_assimp.gpuModel = teapotGpu_assimp;
            teapot_mine.gpuModel = teapotGpu_mine; 
            
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
        const GpuModel* lpsheadGpu_assimp = nullptr;
        // right Backpack, using my loader
        const GpuModel* lpsheadGpu_mine = nullptr;
        
        
        // Make instances of the models 
        ModelInstance lpshead_assimp{}; 
        ModelInstance lpshead_mine{}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoIpshead(
            Assets::ModelCache& cache,
            bool verboseLogs = false)
        {
            Assets::ModelSource source;
            source.filePath = lpsheadPath;
            source.importSettings.flipUVs = true;

            // Keep the two importer results separate for comparison.
            source.importSettings.importer =
                Assets::ModelImporter::Assimp;

            lpsheadGpu_assimp = cache.GetOrLoad(source, verboseLogs);

            source.importSettings.importer =
                Assets::ModelImporter::CustomObj;
            source.importSettings.skipInvalidFaces = true;

            lpsheadGpu_mine = cache.GetOrLoad(source, verboseLogs);

            lpshead_assimp.gpuModel = lpsheadGpu_assimp;
            lpshead_mine.gpuModel = lpsheadGpu_mine; 
            
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
        const GpuModel* cubeGpu_assimp = nullptr;
        // right Backpack, using my loader
        const GpuModel* cubeGpu_mine = nullptr; 
        
        
        // Make instances of the models 
        ModelInstance cube_assimp{}; 
        ModelInstance cube_mine{}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoCubes(
            Assets::ModelCache& cache,
            bool verboseLogs = false)
        {
            Assets::ModelSource source;
            source.filePath = cubePath;
            source.importSettings.flipUVs = true;

            // Keep the two importer results separate for comparison.
            source.importSettings.importer =
                Assets::ModelImporter::Assimp;

            cubeGpu_assimp = cache.GetOrLoad(source, verboseLogs);

            source.importSettings.importer =
                Assets::ModelImporter::CustomObj;
            source.importSettings.skipInvalidFaces = false;

            cubeGpu_mine = cache.GetOrLoad(source, verboseLogs);

            cube_assimp.gpuModel = cubeGpu_assimp;
            cube_mine.gpuModel = cubeGpu_mine; 
            
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
        const GpuModel* pineGpu_assimp = nullptr;  
        // right Backpack, using my loader
        const GpuModel* pineGpu_mine = nullptr; 
        
        
        // Make instances of the models 
        ModelInstance pine_assimp{}; 
        ModelInstance pine_mine{}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoPines(
            Assets::ModelCache& cache,
            bool verboseLogs = false)
        {
            Assets::ModelSource source;
            source.filePath = pinePath;
            source.importSettings.flipUVs = true;

            // Keep the two importer results separate for comparison.
            source.importSettings.importer =
                Assets::ModelImporter::Assimp;

            pineGpu_assimp = cache.GetOrLoad(source, verboseLogs);

            source.importSettings.importer =
                Assets::ModelImporter::CustomObj;
            source.importSettings.skipInvalidFaces = false;

            pineGpu_mine = cache.GetOrLoad(source, verboseLogs);

            pine_assimp.gpuModel = pineGpu_assimp;
            pine_mine.gpuModel = pineGpu_mine; 
            
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
        const GpuModel* sportCarGpu_assimp = nullptr;
        // right Backpack, using my loader
        const GpuModel* sportCarGpu_mine = nullptr;
        
        
        // Make instances of the models 
        ModelInstance sportCar_assimp{}; 
        ModelInstance sportCar_mine{}; 
        
        
        GLuint sharedTexture = 0;
        
        
        Testing_TwoSportCars(
            Assets::ModelCache& cache,
            bool verboseLogs = false)
        {
            Assets::ModelSource source;
            source.filePath = sportCarsPath;
            source.importSettings.flipUVs = true;

            // Keep the two importer results separate for comparison.
            source.importSettings.importer =
                Assets::ModelImporter::Assimp;

            sportCarGpu_assimp = cache.GetOrLoad(source, verboseLogs);

            source.importSettings.importer =
                Assets::ModelImporter::CustomObj;
            source.importSettings.skipInvalidFaces = true;

            sportCarGpu_mine = cache.GetOrLoad(source, verboseLogs);

            sportCar_assimp.gpuModel = sportCarGpu_assimp;
            sportCar_mine.gpuModel = sportCarGpu_mine; 
            
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
    
    
    
    
    
    
    
    
    
}


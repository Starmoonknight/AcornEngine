#include "Demo.h"

// Project headers
#include "AcornMath.h"
#include "rendering/Camera.h"
#include "rendering/ShaderProgram.h"
#include "SceneManager.h"

// Standard library
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <random>
#include <vector>                               

// Third-party libraries
#include <glm/gtc/matrix_transform.hpp>     



namespace 
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
    
    
    // Helper for turning scene struct into scene file, for models with textures 
    AcornData::SceneObjectDescription MakeTexturedPlacement(
        const std::string& modelPath,
        const std::string& texturePath,
        float baseScale,
        bool flipUVs)
    {
        AcornData::SceneObjectDescription object;

        object.source.filePath = modelPath;
        object.source.importSettings.importer =
            Assets::EModelImporter::Assimp;
        object.source.importSettings.flipUVs = flipUVs;

        object.textureFilePath = texturePath;
        object.transform.scale = glm::vec3(baseScale);

        object.appearance.overrideColor = true;
        object.appearance.tintColor = glm::vec3(0.5f);
        object.appearance.tintAlpha = 1.0f;
        object.appearance.tintTextureAlpha = 0.0f;
        object.appearance.textureExpected = true;
        object.appearance.lightingRequested = true;

        return object;
    }
    
    // Helper for turning scene struct into scene file, for lit models that don't have a texture 
    AcornData::SceneObjectDescription MakeSolidPlacement(
    const std::string& modelPath,
    float scale,
    const glm::vec3& color)
    {
        auto object = MakeTexturedPlacement(
            modelPath, "", scale, false);

        object.appearance.textureExpected = false;
        object.appearance.overrideColor = true;
        object.appearance.tintColor = color;
        object.appearance.tintAlpha = 1.0f;
        object.appearance.tintTextureAlpha = 0.0f;

        return object;
    }
    
    
}





namespace Demo
{
    
    

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
    
    
    
    void Update_Transforms(
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
    
    
    
    
    void UpdateModelPair(GLFWwindow* window, GeometryRuntimeValues_Inputs& inputs,
        GeometryRuntimeValues_Transforms& animation, AcornData::RenderModelInstance& left,
        AcornData::RenderModelInstance& right, float deltaTime)
    {
       const glm::vec3 previousTranslation = animation.translation;
       const glm::vec3 previousRotation = animation.rotationDegrees;
       const glm::vec3 previousScale = animation.scale;

       HandleRuntimeInputs(window, inputs, animation);
       Update_Transforms(animation, deltaTime);

       const glm::vec3 translationChange =
           animation.translation - previousTranslation;

       const glm::vec3 rotationChange =
           animation.rotationDegrees - previousRotation;

       const glm::vec3 scaleRatio =
           animation.scale / previousScale;

       for (auto* model : {&left, &right})
       {
           model->transform.position += translationChange;
           model->transform.rotationDegrees += rotationChange;
           model->transform.scale *= scaleRatio;
       }
    }


    void DrawAnimatedModel(
    const ShaderProgram& shader,
    const Rendering::CameraMatrices& viewMatrices,
    const AcornData::RenderModelInstance& model)
   {
       Rendering::DrawModel(shader, viewMatrices, model);
   }
    

    void DrawModelPair(
        const ShaderProgram& shader,
        const Rendering::CameraMatrices& viewMatrices,
        const AcornData::RenderModelInstance& left,
        const AcornData::RenderModelInstance& right)
   {
       DrawAnimatedModel(shader, viewMatrices, left);
       DrawAnimatedModel(shader, viewMatrices, right);
   }

    
    
    
    
    // later move to camera arm, when that exists  
    void SetCameraOrbitPose(
        Rendering::CameraData& camera,
        const glm::vec3& target,
        float angleDegrees,
        glm::vec3 startingOffset)
   {
       const glm::mat4 orbitRotation = glm::rotate(
           glm::mat4(1.0f),
           glm::radians(angleDegrees),
           glm::vec3(0.0f, 1.0f, 0.0f));

       const glm::vec3 rotatedOffset =
           glm::mat3(orbitRotation) * startingOffset;

       camera.SetPosition(target + rotatedOffset);

       const bool aimed = camera.LookAt(
           target,
           glm::vec3(0.0f, 1.0f, 0.0f));

       assert(aimed);
   }
    
    
    // later move to camera arm, when that exists  
    void UpdateOrbitingCamera(SceneCamera& camera, float deltaTime)
   {
       constexpr float orbitDegreesPerSecond = 20.0f;

       camera.orbitAngleDegrees = std::fmod(
           camera.orbitAngleDegrees
               + orbitDegreesPerSecond * deltaTime,
           360.0f);

       SetCameraOrbitPose(
           camera.camera,
           camera.target,
           camera.orbitAngleDegrees,
           camera.startingOffset);
   }

    
    void UpdateCameraPos(
    GLFWwindow* window,
    Rendering::CameraData& cameraData,
    float speedUnitsPerSecond,
    float deltaTime)
   {
       const float distance = speedUnitsPerSecond * deltaTime;

       if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
       {
           cameraData.TranslateLocalZ(-distance); // Forward
       }

       if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
       {
           cameraData.TranslateLocalZ(distance); // Backward
       }

       if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
       {
           cameraData.TranslateLocalX(-distance); // Left
       }

       if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
       {
           cameraData.TranslateLocalX(distance); // Right
       }
   }
    

    bool CreatePlayers(Scene& scene, GLuint textureID, const std::string& filePath)
    {
        if (scene.cameras.size() != scene.players.size())
            return false;

        Assets::ModelSource source;
        source.filePath = filePath; 
        source.importSettings.importer = Assets::EModelImporter::Assimp;

        const auto* geometry = scene.modelCache.GetOrLoad(source);

        if (geometry == nullptr)
            return false;

        for (std::size_t i = 0; i < scene.players.size(); ++i)
        {
            Player& player = scene.players[i];
            SceneCamera& view = scene.cameras[i];

            player.cameraIndex = i;
            player.position = scene.playerStarts[i].position;

            view.camera.SetRotationDegrees(
                scene.playerStarts[i].rotationDegrees);

            player.orientation = view.camera.orientation;

            player.body.gpuModel = geometry;
            player.body.textureID = textureID;
            player.body.transform.scale = glm::vec3(0.5f, 1.0f, 0.5f);
            player.body.renderSettings.textureExpected = textureID != 0;
           
            player.modelOrientationCorrection = glm::angleAxis(
                glm::radians(180.0f), 
                glm::vec3(0.0f, 1.0f, 0.0f));
            
            player.body.renderSettings.lightingRequested = true;
            
            if (i == 0)
            {
                player.body.renderSettings.overrideColor = true;
                player.body.renderSettings.tintColor = Palette::orange;
                player.body.renderSettings.tintAlpha = 1.0f;
                player.body.renderSettings.tintTextureAlpha = 1.0f;
            }
            else if (i == 1)
            {
                player.body.renderSettings.overrideColor = true;
                player.body.renderSettings.tintColor = Palette::blue;
                player.body.renderSettings.tintAlpha = 1.0f;
                player.body.renderSettings.tintTextureAlpha = 1.0f;
            }
            

            player.SyncViewAndBody(view.camera);

            scene.RegisterRenderInstance(&player.body);
            view.hiddenInstance = &player.body;
        }

        scene.playersReady = true;
        return true;
    }


    std::array<AcornData::SceneDescription, 3> MakeDemoScenes()
    {
        std::array<AcornData::SceneDescription, 3> scenes{};

        for (std::size_t i = 0; i < scenes.size(); ++i)
        {
            auto& scene = scenes[i];

            scene.name = "Demo " + std::to_string(i + 1);
            scene.sceneID = static_cast<int>(i);
            
            if (i == 2)
            {
                scene.settings.splitScreen =
                    AcornData::ESplitScreenSettings::HorizontalSplit;
            }
            else
            {
                scene.settings.splitScreen =
                    AcornData::ESplitScreenSettings::VerticalSplit;
            }

            const float startHeight = 1.0f + static_cast<float>(i);

            scene.playerStarts[0].position =
                glm::vec3(-2.0f, startHeight, 8.0f);

            scene.playerStarts[1].position =
                glm::vec3(2.0f, startHeight, 8.0f);

            // Both use zero rotation and therefore face world -Z.
            scene.goalPosition = glm::vec3(5.0f, 0.0f, -5.0f);

            for (int objectIndex = 0; objectIndex < 3; ++objectIndex)
            {
                AcornData::SceneObjectDescription object;

                
                object.source.filePath = "models/cube/cube.obj";
                object.source.importSettings.importer =
                    Assets::EModelImporter::Assimp;

                object.transform.position = glm::vec3(
                    static_cast<float>(objectIndex - 1) * 3.0f,
                    0.0f,
                    -static_cast<float>(i) * 3.0f);

                object.appearance.overrideColor = true;
                object.appearance.lightingRequested = true;
                object.appearance.textureExpected = false;

                const std::array<glm::vec3, 3> colors{
                    glm::vec3(0.9f, 0.2f, 0.2f),
                    glm::vec3(0.2f, 0.9f, 0.2f),
                    glm::vec3(0.2f, 0.2f, 0.9f)
                };

                object.appearance.tintColor = colors[i];

                scene.objects.push_back(object);
            }
            
            //A floor 
            AcornData::SceneObjectDescription floor;
            
            floor.source.filePath = "models/plane/plane.obj";
            floor.source.importSettings.importer =
                Assets::EModelImporter::Assimp;
            
            floor.transform.position = glm::vec3(0.0f, -10.0f, 0.0f);
            floor.transform.scale = glm::vec3(30.0f, 1.0f, 30.0f);

            floor.appearance.overrideColor = true;
            floor.appearance.tintAlpha = 1.0f;
            floor.appearance.tintColor = Palette::grey; 
            floor.appearance.lightingRequested = true;
            
            floor.textureFilePath =
                "textures/cobblestone_floor_07_diff_4k.jpg";
            floor.appearance.textureExpected = true;
            
            scene.objects.push_back(floor);
            
            
            // Visual marker for the logical goal location.
            // Its mesh is only a marker; completion uses the distance check in main.
            AcornData::SceneObjectDescription goalMarker;

            goalMarker.source.filePath = "models/cube/cube.obj";
            goalMarker.source.importSettings.importer =
                Assets::EModelImporter::Assimp;

            goalMarker.transform.position = scene.goalPosition;
            goalMarker.transform.scale = glm::vec3(0.3f);

            goalMarker.appearance.overrideColor = true;
            goalMarker.appearance.tintColor = glm::vec3(1.0f, 1.0f, 0.0f);
            goalMarker.appearance.tintAlpha = 1.0f;
            goalMarker.appearance.textureExpected = false;
            goalMarker.appearance.lightingRequested = false;

            scene.objects.push_back(goalMarker);
        }

        return scenes;
    }
    
    


    void ApplyDemoAppearance_Backpack(SceneObject& object, GLuint backpackTexture)
    {
        if (object.source.filePath == "models/backpack/backpack.obj")
        {
            object.instance.renderSettings.textureExpected = true;
            object.instance.textureID = backpackTexture;
        }
    }

    void ApplyDemoAppearance_Teapot(SceneObject& object, GLuint teapotTexture)
    {
        if (object.source.filePath == "models/teapot/teapot.obj")
        {
            object.instance.renderSettings.textureExpected = true;
            object.instance.textureID = teapotTexture;
        }
    }

    void ApplyDemoAppearance_Ipshead(SceneObject& object, GLuint ipsheadTexture)
    {
        if (object.source.filePath == "models/lpshead/head.obj")
        {
            object.instance.renderSettings.textureExpected = true;
            object.instance.textureID = ipsheadTexture;
        }
    }

    void ApplyDemoAppearance_Cube(SceneObject& object, GLuint cubeTexture)
    {
        if (object.source.filePath == "models/cube/cube.obj")
        {
            object.instance.renderSettings.textureExpected = true;
            object.instance.textureID = cubeTexture;
        }
    }

    void ApplyDemoAppearance_Pine(SceneObject& object, GLuint pineTexture)
    {
        if (object.source.filePath == "models/pine/scrubPine.obj")
        {
            object.instance.renderSettings.textureExpected = true;
            object.instance.textureID = pineTexture;
        }
    }

    
    void PopulateComparison_Backpack(Scene& loaded, GLuint textureID)
    {
        AcornData::SceneObjectDescription description; 

        description.source.filePath = "models/backpack/backpack.obj";
        description.source.importSettings.flipUVs = true;
        
        description.appearance.overrideColor = true;
        description.appearance.tintColor = glm::vec3(0.75f);    // light gray color without texture 
        description.appearance.lightingRequested = true;
        description.appearance.textureExpected = true;

        
        // Left object: Assimp interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::Assimp;

        description.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);

        
        // Right object: custom OBJ interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::CustomObj;
        description.source.importSettings.skipInvalidFaces = false;

        description.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);
    }

    
    void PopulateComparison_Teapot(Scene& loaded, GLuint textureID)
    {
        AcornData::SceneObjectDescription description; 

        description.source.filePath = "models/teapot/teapot.obj";
        description.source.importSettings.flipUVs = true;
        
        description.appearance.overrideColor = true;
        description.appearance.tintColor = glm::vec3(0.75f);    // light gray color without texture 
        description.appearance.lightingRequested = true;
        description.appearance.textureExpected = true;
        
        description.transform.scale = glm::vec3(0.025f);
        
        
        // Left object: Assimp interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::Assimp;

        description.transform.position = glm::vec3(-3.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);

        
        // Right object: custom OBJ interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::CustomObj;
        description.source.importSettings.skipInvalidFaces = false;

        description.transform.position = glm::vec3(3.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);
    }

    
    void PopulateComparison_Ipshead(Scene& loaded, GLuint textureID)
    {
        AcornData::SceneObjectDescription description; 

        description.source.filePath = "models/lpshead/head.obj";
        description.source.importSettings.flipUVs = true;
        
        description.appearance.overrideColor = true;
        description.appearance.tintColor = glm::vec3(0.75f);    // light gray color without texture 
        description.appearance.lightingRequested = true;
        description.appearance.textureExpected = true;
        
        description.transform.scale = glm::vec3(10.0f);

        
        // Left object: Assimp interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::Assimp;
        
        description.transform.position = glm::vec3(-2.0f, 2.0f, 0.0f);

        loaded.AddObject(description, textureID);

        
        // Right object: custom OBJ interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::CustomObj;
        description.source.importSettings.skipInvalidFaces = false;

        description.transform.position = glm::vec3(2.0f, 2.0f, 0.0f);

        loaded.AddObject(description, textureID);
    }

    
    void PopulateComparison_Cube(Scene& loaded, GLuint textureID)
    {
        AcornData::SceneObjectDescription description; 

        description.source.filePath = "models/cube/cube.obj";
        description.source.importSettings.flipUVs = true;
        
        description.appearance.overrideColor = true;
        description.appearance.tintColor = glm::vec3(0.75f);    // light gray color without texture 
        description.appearance.lightingRequested = true;
        description.appearance.textureExpected = true;
        
        description.transform.scale = glm::vec3(2.0f);

        
        // Left object: Assimp interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::Assimp;
        
        description.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);

        
        // Right object: custom OBJ interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::CustomObj;
        description.source.importSettings.skipInvalidFaces = false;

        description.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);
    }

    
    void PopulateComparison_Pine(Scene& loaded, GLuint textureID)
    {
        AcornData::SceneObjectDescription description; 

        description.source.filePath = "models/pine/scrubPine.obj";
        description.source.importSettings.flipUVs = true;
        
        description.appearance.overrideColor = true;
        description.appearance.tintColor = glm::vec3(0.75f);    // light gray color without texture 
        description.appearance.lightingRequested = true;
        description.appearance.textureExpected = true;
        
        description.transform.scale = glm::vec3(0.01f);

        
        // Left object: Assimp interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::Assimp;
        
       description.transform.position = glm::vec3(-3.0f, -2.0f, 0.5f);

        loaded.AddObject(description, textureID);

        
        // Right object: custom OBJ interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::CustomObj;
        description.source.importSettings.skipInvalidFaces = false;

        description.transform.position = glm::vec3(3.0f, -2.0f, 0.5f);
        
        loaded.AddObject(description, textureID);
    }

    
    void PopulateComparison_SportsCar(Scene& loaded, GLuint textureID)
    {
        AcornData::SceneObjectDescription description; 

        description.source.filePath = "models/sportsCar/sportsCar.obj";
        description.source.importSettings.flipUVs = true;
        
        description.appearance.overrideColor = true;
        description.appearance.tintColor = glm::vec3(0.75f);    // light gray color without texture, remember to add to obj loader as default color when empty 
        description.appearance.lightingRequested = true;
        description.appearance.textureExpected = false; 
        

        // Left object: Assimp interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::Assimp;
        
        description.transform.position = glm::vec3(-2.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);

        
        // Right object: custom OBJ interpretation.
        description.source.importSettings.importer =
            Assets::EModelImporter::CustomObj;
        description.source.importSettings.skipInvalidFaces = false;

        description.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);

        loaded.AddObject(description, textureID);
    }
    
    
    
    
    
    
    
    // --------------- Making Scenes --------------------
    
    
    // The resulting forest has 64 placed objects: ground, 60 trees, two cars, and the goal marker.
    AcornData::SceneDescription ForestScene::Build()
    {
        AcornData::SceneDescription scene;
        scene.name = "Autumn Forest";
        scene.sceneID = 0;
        scene.settings.defaultBackgroundColor =
            glm::vec3(0.45f, 0.55f, 0.65f);

        constexpr float groundY = -10.0f;

        // The actual plane spans -1..+1 in X/Z.
        // Scale 60 gives a 120 x 120 ground, twice the old width/depth.
        auto ground = MakeTexturedPlacement(
            "models/plane/plane.obj",
            "textures/cobblestone_floor_07_diff_4k.jpg",
            1.0f,
            false);

        ground.transform.position = glm::vec3(0.0f, groundY, 0.0f);
        ground.transform.scale = glm::vec3(60.0f, 1.0f, 60.0f);
        scene.objects.push_back(ground);

        // Begin near the +Z edge, facing -Z through the forest.
        scene.playerStarts[0].position =
            glm::vec3(-3.0f, groundY + 1.0f, 48.0f);
        scene.playerStarts[1].position =
            glm::vec3(3.0f, groundY + 1.0f, 48.0f);

        // Closer to the center, while still beyond much of the forest.
        scene.goalPosition = glm::vec3(0.0f, groundY + 1.0f, -25.0f);

        const std::array<glm::vec3, 6> autumnColors{
            glm::vec3(1.0f, 0.85f, 0.1f),
            glm::vec3(1.0f, 0.55f, 0.1f),
            glm::vec3(0.95f, 0.15f, 0.1f),
            glm::vec3(0.8f, 0.1f, 0.25f),
            glm::vec3(1.0f, 0.35f, 0.6f),
            glm::vec3(1.0f, 0.65f, 0.75f)
        };

        // Fixed seed: repeated previews preserve the same layout.
        std::mt19937 random(1042);
        std::uniform_real_distribution<float> jitter(-3.0f, 3.0f);
        std::uniform_real_distribution<float> sizeMultiplier(0.8f, 1.2f);
        std::uniform_real_distribution<float> yaw(0.0f, 360.0f);

        // Measured raw OBJ minimum Y is approximately 3.03.
        // At scale 0.01, that is only about 0.03 world units.
        constexpr float modelMinY = 3.03f;
        constexpr float treeHeightAdjustment = 0.0f;

            
        // Placement of forrest 
        for (int row = 0; row < 10; ++row)
        {
            for (int column = 0; column < 6; ++column)
            {
                const int treeIndex = row * 6 + column;

                auto tree = MakeTexturedPlacement(
                    "models/pine/scrubPine.obj",
                    "models/pine/LushPine.png",
                    0.01f,
                    true);

                const float scale = 0.01f * sizeMultiplier(random);
                tree.transform.scale = glm::vec3(scale);
                tree.transform.rotationDegrees.y = yaw(random);

                tree.transform.position = glm::vec3(
                    -40.0f + column * 16.0f + jitter(random),
                    groundY - modelMinY * scale + treeHeightAdjustment,
                    36.0f - row * 8.0f + jitter(random));

                tree.appearance.tintColor =
                    autumnColors[treeIndex % autumnColors.size()];
                tree.appearance.tintTextureAlpha = 0.75f;

                // One tree near the starting end intentionally lacks a texture.
                if (treeIndex == 2)
                    tree.textureFilePath.clear();

                // Keep textureExpected true, including for that one tree.
                scene.objects.push_back(tree);
            }
        }
        
        
        // Placement of sports cars
        std::uniform_real_distribution<float> carX(-35.0f, 35.0f);
        std::uniform_real_distribution<float> carZ(-30.0f, 30.0f);

        for (int i = 0; i < 2; ++i)
        {
            // Existing sports-car preset uses scale 1.
            auto car = MakeSolidPlacement(
                "models/sportsCar/sportsCar.obj",
                1.0f,
                glm::vec3(0.9f, 0.04f, 0.03f));

            // This model's lowest vertices are approximately at local Y = 0.
            car.transform.position =
                glm::vec3(carX(random), groundY, carZ(random));

            car.transform.rotationDegrees.y = yaw(random);

            scene.objects.push_back(car);
        }

        AcornData::SceneObjectDescription goal;
        goal.source.filePath = "models/cube/cube.obj";
        goal.transform.position = scene.goalPosition;
        goal.transform.scale = glm::vec3(0.3f);
        goal.appearance.overrideColor = true;
        goal.appearance.tintColor = glm::vec3(1.0f, 1.0f, 0.0f);
        goal.appearance.lightingRequested = false;
        scene.objects.push_back(goal);

        return scene;
    }
        
    
    // Teapots: 80 placements throughout a volume 
    AcornData::SceneDescription TeapotScene::Build()
    {
        AcornData::SceneDescription scene;
        scene.name = "Find the Green Teapot";
        scene.sceneID = 1;
        scene.settings.defaultBackgroundColor =
            glm::vec3(0.06f, 0.08f, 0.14f);
        scene.settings.splitScreen =
            AcornData::ESplitScreenSettings::HorizontalSplit;

        // Start between two layers of teapots.
        scene.playerStarts[0].position = glm::vec3(-2.0f, 0.0f, 0.0f);
        scene.playerStarts[1].position = glm::vec3(2.0f, 0.0f, 0.0f);

        std::mt19937 random(2057);

        std::uniform_real_distribution<float> randomX(-25.0f, 25.0f);
        std::uniform_real_distribution<float> randomY(-20.0f, 20.0f);
        std::uniform_real_distribution<float> randomZ(-25.0f, 25.0f);

        std::uniform_real_distribution<float> sizeMultiplier(0.5f, 1.8f);
        std::uniform_real_distribution<float> angle(-180.0f, 180.0f);

        constexpr int goalTeapotIndex = 17;

        for (int i = 0; i < 80; ++i)
        {
            auto teapot = MakeTexturedPlacement(
                "models/teapot/teapot.obj",
                "models/teapot/default.png",
                0.025f,
                true);

            glm::vec3 position;

            // Keep a small clear region around the starting players.
            // Elsewhere, overlapping teapots are allowed.
            do
            {
                position = glm::vec3(
                    randomX(random),
                    randomY(random),
                    randomZ(random));
            }
            while (glm::dot(position, position) < 8.0f * 8.0f);

            teapot.transform.position = position;

            teapot.transform.scale =
                glm::vec3(0.025f * sizeMultiplier(random));

            teapot.transform.rotationDegrees =
                glm::vec3(angle(random), angle(random), angle(random));

            teapot.appearance.tintColor =
                i == goalTeapotIndex
                ? glm::vec3(0.05f, 1.0f, 0.05f)
                : glm::vec3(1.0f, 0.05f, 0.05f);

            teapot.appearance.tintTextureAlpha = 1.0f;

            if (i == goalTeapotIndex)
            {
                // The mesh extends upward from its origin.
                // Put the logical goal near the transformed mesh center,
                // rather than at the rotated teapot's base.
                const glm::vec3 localCenter(5.2f, 39.5f, 0.0f);

                scene.goalPosition =
                    teapot.transform.position
                    + AcornMath::MakeOrientationXYZ(
                        teapot.transform.rotationDegrees)
                    * (localCenter * teapot.transform.scale);
            }

            scene.objects.push_back(teapot);
        }

        return scene;
    }

    
    
    // This produces 55 placed objects: 36 pillars, 18 exhibits, and one yellow goal marker.
    AcornData::SceneDescription PillarScene::Build()
    {
        AcornData::SceneDescription scene;
        scene.name = "Basalt Gallery";
        scene.sceneID = 2;
        scene.settings.defaultBackgroundColor =
            glm::vec3(0.35f, 0.42f, 0.5f);

        constexpr int columns = 6;
        constexpr int rows = 6;
        constexpr int pillarCount = columns * rows;

        // Testing cube scale 2, multiplied by 5.
        constexpr float pillarWidth = 10.0f;

        // Common base; varying heights determine the top surface.
        constexpr float bottomY = -18.0f;

        std::mt19937 random(3081);
        std::uniform_real_distribution<float> heightVariation(6.0f, 9.0f);
        std::uniform_real_distribution<float> yaw(0.0f, 360.0f);

        auto pillarCenter = [](int index)
        {
            const int column = index % columns;
            const int row = index / columns;

            return glm::vec3(
                -25.0f + column * pillarWidth,
                0.0f,
                25.0f - row * pillarWidth);
        };

        // Reserve two starting pillars and one goal pillar.
        constexpr int player1Pillar = 0;
        constexpr int player2Pillar = 1;
        constexpr int goalPillar = pillarCount - 1;

        std::vector<int> availablePillars;

        for (int i = 0; i < pillarCount; ++i)
        {
            if (i != player1Pillar &&
                i != player2Pillar &&
                i != goalPillar)
            {
                availablePillars.push_back(i);
            }
        }

        std::shuffle(
            availablePillars.begin(), availablePillars.end(), random);

        // Each exhibit gets its own raised pillar.
        std::array<bool, pillarCount> hasExhibit{};

        for (int i = 0; i < 18; ++i)
            hasExhibit[availablePillars[i]] = true;

        std::array<float, pillarCount> topHeights{};

        for (int i = 0; i < pillarCount; ++i)
        {
            const int row = i / columns;

            const float height =
                heightVariation(random) + (hasExhibit[i] ? 2.5f : 0.0f);

            topHeights[i] = bottomY + height;

            // Row 0 is closest to +Z; last row is closest to -Z.
            const float brightness =
                0.15f + 0.60f * static_cast<float>(row) / (rows - 1);

            auto pillar = MakeSolidPlacement(
                "models/cube/cube.obj",
                1.0f,
                glm::vec3(brightness));

            pillar.transform.scale =
                glm::vec3(pillarWidth, height, pillarWidth);

            pillar.transform.position = pillarCenter(i);

            // Cube geometry is centered at its origin.
            // Half the height below this position reaches bottomY.
            pillar.transform.position.y = bottomY + height * 0.5f;

            scene.objects.push_back(pillar);
        }

        struct ExhibitPreset
        {
            const char* model;
            const char* texture;
            float scale;
            float localMinY;
            bool flipUVs;
        };

        // Existing test scales. Minimum Y values measured from the OBJ files,
        // rounded slightly: suitable starting points for visual adjustment.
        const std::array<ExhibitPreset, 6> presets{
            ExhibitPreset{
                "models/backpack/backpack.obj",
                "models/backpack/diffuse.jpg",
                1.0f, -1.74f, true
            },
            ExhibitPreset{
                "models/teapot/teapot.obj",
                "models/teapot/default.png",
                0.025f, 0.0f, true
            },
            ExhibitPreset{
                "models/lpshead/head.OBJ",
                "models/lpshead/rendered.jpg",
                10.0f, -0.31f, true
            },
            ExhibitPreset{
                "models/pine/scrubPine.obj",
                "models/pine/LushPine.png",
                0.01f, 3.03f, true
            },
            ExhibitPreset{
                "models/sportsCar/sportsCar.obj",
                "",
                1.0f, 0.0f, false
            },
            ExhibitPreset{
                "models/suzanne/suzanne.obj",
                "",
                1.0f, -0.98f, false
            }
        };

        const std::array<glm::vec3, 7> rainbow{
            glm::vec3(1.0f, 0.05f, 0.05f),
            glm::vec3(1.0f, 0.45f, 0.05f),
            glm::vec3(1.0f, 0.9f, 0.05f),
            glm::vec3(0.1f, 0.85f, 0.2f),
            glm::vec3(0.1f, 0.4f, 1.0f),
            glm::vec3(0.4f, 0.15f, 0.85f),
            glm::vec3(0.9f, 0.15f, 0.8f)
        };

        std::array<int, 7> rainbowOrder{0, 1, 2, 3, 4, 5, 6};
        std::shuffle(rainbowOrder.begin(), rainbowOrder.end(), random);

        int exhibitIndex = 0;

        for (std::size_t type = 0; type < presets.size(); ++type)
        {
            const auto& preset = presets[type];

            for (int copy = 0; copy < 3; ++copy)
            {
                const int support = availablePillars[exhibitIndex++];

                auto object = MakeTexturedPlacement(
                    preset.model,
                    preset.texture,
                    preset.scale,
                    preset.flipUVs);

                object.appearance.textureExpected =
                    !object.textureFilePath.empty();

                // Yaw does not change minimum Y, keeping grounding simple.
                object.transform.rotationDegrees.y = yaw(random);

                object.transform.position = pillarCenter(support);
                object.transform.position.y =
                    topHeights[support]
                    - preset.localMinY * preset.scale;

                if (type == 4) // Sports car
                {
                    object.appearance.tintColor =
                        glm::vec3(0.9f, 0.04f, 0.03f);
                }
                else if (type == 5) // Suzanne
                {
                    object.appearance.tintColor =
                        rainbow[rainbowOrder[copy]];
                }

                scene.objects.push_back(object);
            }
        }

        scene.playerStarts[0].position = pillarCenter(player1Pillar);
        scene.playerStarts[0].position.y = topHeights[player1Pillar] + 1.0f;

        scene.playerStarts[1].position = pillarCenter(player2Pillar);
        scene.playerStarts[1].position.y = topHeights[player2Pillar] + 1.0f;

        scene.goalPosition = pillarCenter(goalPillar);
        scene.goalPosition.y = topHeights[goalPillar] + 1.0f;

        auto goal = MakeSolidPlacement(
            "models/cube/cube.obj",
            0.3f,
            glm::vec3(1.0f, 1.0f, 0.0f));

        goal.transform.position = scene.goalPosition;
        goal.appearance.lightingRequested = false;
        scene.objects.push_back(goal);

        return scene;
    }
    
    
    
    
    
    
    
}

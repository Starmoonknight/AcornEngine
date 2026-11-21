#include "Demo.h"

// Project headers
#include "rendering/Camera.h"
#include "SceneManager.h"



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
    
    
    
    
    void Update_TestingDefaultShapes(GLFWwindow* window, Testing_DefaultShapes& basicShapes, float deltaTime)
    {
       
       // Capture the animation values currently reflected in the models BEFORE input: 
       // Space can reset the manual values.
       const glm::vec3 previousManualTranslation = basicShapes.runtime_Info.translation;
       const glm::vec3 previousManualRotation = basicShapes.runtime_Info.rotationDegrees;
       const glm::vec3 previousManualScale = basicShapes.runtime_Info.scale;

       const glm::vec3 previousPyramidScale = basicShapes.pyramidAuto.scale;
       const glm::vec3 previousCubeRotation = basicShapes.cubeAuto.rotationDegrees;
       const glm::vec3 previousSphereTranslation = basicShapes.sphereAuto.translation;
       
        
        HandleRuntimeInputs(window, basicShapes.runtime_Inputs, basicShapes.runtime_Info);

        Update_Transforms(basicShapes.runtime_Info, deltaTime);
        Update_Transforms(basicShapes.cubeAuto, deltaTime);
        Update_Transforms(basicShapes.sphereAuto, deltaTime);
        Update_Transforms(basicShapes.pyramidAuto, deltaTime);
        
        
       
       // Translation and Euler-angle offsets combine by addition,
       // so their per-update changes are differences.
       const glm::vec3 manualTranslationChange =
           basicShapes.runtime_Info.translation - previousManualTranslation;

       const glm::vec3 manualRotationChange =
           basicShapes.runtime_Info.rotationDegrees - previousManualRotation;

       const glm::vec3 cubeRotationChange =
           basicShapes.cubeAuto.rotationDegrees - previousCubeRotation;

       const glm::vec3 sphereTranslationChange =
           basicShapes.sphereAuto.translation - previousSphereTranslation;

       
       // Scale factors combine by multiplication,
       // so their per-update changes are ratios.
       const glm::vec3 manualScaleRatio =
           basicShapes.runtime_Info.scale / previousManualScale;

       const glm::vec3 pyramidScaleRatio =
           basicShapes.pyramidAuto.scale / previousPyramidScale;
       
       
       
       // Automatically scaling pyramids.
       basicShapes.basicPyramid_1.transform.scale *= pyramidScaleRatio;
       basicShapes.basicPyramid_2.transform.scale *= pyramidScaleRatio;
       basicShapes.pyramid_V1.transform.scale *= pyramidScaleRatio;
       
       
       // Manually controlled pyramid.
       basicShapes.pyramid_manual.transform.position +=
           manualTranslationChange;
       basicShapes.pyramid_manual.transform.rotationDegrees +=
           manualRotationChange;
       basicShapes.pyramid_manual.transform.scale *=
           manualScaleRatio;
       
       
       // Automatically rotating cubes.
       basicShapes.basicCube_1.transform.rotationDegrees +=
           cubeRotationChange;
       basicShapes.basicCube_2.transform.rotationDegrees -=
           cubeRotationChange;
       basicShapes.cube_V2.transform.rotationDegrees +=
           cubeRotationChange;

       
       // Automatically translating spheres.
       basicShapes.sphere_1.transform.position +=
           sphereTranslationChange;
       basicShapes.sphere_2.transform.position -=
           sphereTranslationChange;
       
    }
    
    
    void Draw_TestingDefaultShapes(const ShaderProgram& shaderProgram, Testing_DefaultShapes& basicShapes, const Rendering::CameraMatrices& viewMatrices)
    {
        
        // activate program
        shaderProgram.use(); 
        
        const BasicUniformLocations& uniforms = shaderProgram.GetBasicUniformLocations();
        glUniform1i(uniforms.textureLocation, 0); // uTexture2D → unit 0
        
        
        // Pyramid: placed on the left.
        Rendering::DrawModel(shaderProgram, viewMatrices, basicShapes.basicPyramid_1);
       
        Rendering::DrawModel(shaderProgram, viewMatrices, basicShapes.basicPyramid_2);
        
        
        Rendering::DrawModel(shaderProgram, viewMatrices, basicShapes.pyramid_V1);
        
        
        // manually controlled pyramid,     testing drawing colors per face 
        glm::mat4 pyramidMatrix_Manual = AcornMath::BuildModelMatrix(basicShapes.pyramid_manual.transform);
        assert(basicShapes.pyramid_manual.gpuModel != nullptr);
        assert(basicShapes.pyramid_manual.gpuModel->meshes.size() == 1);
        const auto& manualPyramidMesh = basicShapes.pyramid_manual.gpuModel->meshes.front();
        Rendering::SetBasicShaderUniforms(shaderProgram, pyramidMatrix_Manual, viewMatrices.view, viewMatrices.projection, 
            basicShapes.pyramid_manual.renderSettings.overrideColor, basicShapes.pyramid_manual.renderSettings.tintColor, 1.0f, false, 0.0f
        );
        shaderProgram.SetBool("uUseLighting", manualPyramidMesh.hasNormals);
        Rendering::DrawMesh_FaceColors(shaderProgram, manualPyramidMesh, pyramid_FaceColors); 
        
        
        
        // Cube version 1: placed in the middle.
        Rendering::DrawModel(shaderProgram, viewMatrices, basicShapes.basicCube_1);
        
        
        // testing drawing colors per face 
        glm::mat4 cubeMatrix_2 = AcornMath::BuildModelMatrix(basicShapes.basicCube_2.transform);
        assert(basicShapes.basicCube_2.gpuModel != nullptr);
        assert(basicShapes.basicCube_2.gpuModel->meshes.size() == 1);
        const auto& faceColoredCubeMesh = basicShapes.basicCube_2.gpuModel->meshes.front();
        Rendering::SetBasicShaderUniforms(shaderProgram, cubeMatrix_2, viewMatrices.view, viewMatrices.projection, 
            basicShapes.basicCube_2.renderSettings.overrideColor, basicShapes.basicCube_2.renderSettings.tintColor, 1.0f, false, 0.0f
        );
        shaderProgram.SetBool("uUseLighting", false);
        Rendering::DrawMesh_FaceColors(shaderProgram, faceColoredCubeMesh, cube_FaceColors); 
        
        
        
        // Cube version 2: placed behind the other cubes.  Testing textures 
        Rendering::DrawModel(shaderProgram, viewMatrices, basicShapes.cube_V2); 
        
        
        
        // Sphere: placed on the right.
        Rendering::DrawModel(shaderProgram, viewMatrices, basicShapes.sphere_1);
        
        Rendering::DrawModel(shaderProgram, viewMatrices, basicShapes.sphere_2);
        
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
    
    
}

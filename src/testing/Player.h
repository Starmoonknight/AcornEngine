#pragma once

// Project headers 
#include "rendering/Camera.h"
#include "rendering/RenderInstances.h"
#include "Utility/InputContext.h"

// Standard library 
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>



struct Player
{
    // Scene-owned camera index; not a persistent camera reference.
    std::size_t cameraIndex = 0;

    // Position is body origin, not eye position 
    glm::vec3 position{0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};

    // Local offset from player origin to the eyes.
    glm::vec3 eyeOffset{0.0f, 0.75f, 0.0f};

    float moveSpeed = 8.0f;
    float lookSpeed = 90.0f; // Degrees per second.

    // Owned instance; geometry and texture remain borrowed.
    //  - geometry is in the scene cache 
    //  - texture is applied through hard coded for now  
    AcornData::ModelInstance body;
    
    // Converts the imported model's facing direction into our -Z-forward
    // convention. Change this per model; identity means no correction.
    glm::quat modelOrientationCorrection{
        1.0f, 0.0f, 0.0f, 0.0f
    };
    
    void SyncViewAndBody(Rendering::CameraData& camera)
    {
        camera.SetOrientation(orientation);

        // Rotate the local head offset into world space.
        // Body scale deliberately does not scale the eye offset.
        camera.SetPosition(position + orientation * eyeOffset);

        body.transform.position = position;

        
        // First correct the imported model's local axes,
        // then apply the player's world orientation.
        const glm::quat bodyOrientation =
            orientation * modelOrientationCorrection;
        
        Rendering::CameraData bodyPose;
        bodyPose.SetOrientation(bodyOrientation);

        body.transform.rotationDegrees = bodyPose.GetRotationDegrees();
    }

    void Update(
        const Input::PlayerInput& input,
        Rendering::CameraData& camera,
        float deltaTime)
    {
        camera.SetOrientation(orientation);

        camera.ApplyLookDelta(
            glm::vec2(input.lookRight, input.lookUp)
            * lookSpeed * deltaTime);

        orientation = camera.orientation;

        // Camera-local -Z is forward.
        glm::vec3 localMovement{
            input.right,
            input.up,
            -input.forward
        };

        // Prevent diagonals from moving faster than a single direction.
        const float lengthSquared =
            glm::dot(localMovement, localMovement);

        if (lengthSquared > 1.0f)
            localMovement /= glm::sqrt(lengthSquared);

        position += orientation * localMovement * moveSpeed * deltaTime;

        SyncViewAndBody(camera);
    }
};
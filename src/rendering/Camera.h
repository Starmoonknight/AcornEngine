#pragma once


// Standard library
#include <glm/glm.hpp>                          // Vectors, matrices, dot and cross products
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>


namespace Rendering
{
       
    
    enum Perspective
    {
        PROJECTION,
        ORTHOGRAPHIC,
    };

    struct CameraData
    {
        float nearPlane;
        float farPlane;
        float FOV = 45.0f;
        float aspectRatio;
        Perspective perspective = PROJECTION;
        
        float pitch = 0.0f;
        float yaw = 0.0f;
        float roll = 0.0f;
        
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f);
        glm::vec3 cameraPosition = glm::vec3(0.0f);
        
        glm::vec3 cameraFront;
        glm::vec3 cameraRight;
        glm::vec3 cameraUp;
        
        glm::mat4 view ;
        
        glm::mat4 projection;
        
        CameraData()
        {
            this = RefreshCameraData(this); 
        }
    };
    
    void RefreshCameraData(CameraData& data)
    {
        // Direction from the camera toward its initial target.
        glm::vec3 cameraFront = glm::normalize(data.cameraTarget - data.cameraPosition);
        // Camera's right direction, using the world's up direction.
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, data.worldUp));
        // Camera's up direction, perpendicular to its right and front.
        glm::vec3 cameraUp = glm::cross(cameraRight, cameraFront);
        
        glm::mat4 view = glm::lookAt(data.cameraPosition, data.cameraPosition + cameraFront, cameraUp);
        
        glm::mat4 projection = (data.perspective == PROJECTION) 
        ? glm::perspective(glm::radians(data.FOV), data.aspectRatio, data.nearPlane, data.farPlane)
        : glm::ortho(-5.0f * data.aspectRatio, 5.0f * data.aspectRatio, -5.0f, 5.0f, data.nearPlane, data.farPlane);
    }
    
    
    
}


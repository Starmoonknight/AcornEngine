#pragma once

// Third-party libraries
#include <glm/glm.hpp>          // Vectors, matrices, dot and cross products



namespace AcornData
{
    
    struct Transform
    {
        glm::vec3 position{0.0f};
        glm::vec3 rotationDegrees{0.0f};
        glm::vec3 scale{1.0f};
    };
    
}




namespace AcornMath
{
    
    glm::mat4 BuildModelMatrix(
        glm::vec3 position,
        glm::vec3 scale,
        glm::vec3 rotationDegrees);

    glm::mat4 BuildModelMatrix(const AcornData::Transform& transform);
    
}


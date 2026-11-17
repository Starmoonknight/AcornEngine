#include "AcornMath.h"

// Third-party libraries
#include <glm/gtc/matrix_transform.hpp>         // Model, view, and projection transforms


namespace AcornMath
{
    
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees)
    {
        // A starting matrix that makes no change.
        const glm::mat4 identity(1.0f);
        
        // Describe the object's size.
        glm::mat4 scaleMatrix = glm::scale(
            identity, 
            scale
        );
        
        // Describe rotation around the X axis.
        glm::mat4 rotationMatrix_X = glm::rotate(
            identity, 
            glm::radians(rotationDegrees.x), 
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
        
        // Describe rotation around the Y axis.
        glm::mat4 rotationMatrix_Y = glm::rotate(
            identity, 
            glm::radians(rotationDegrees.y), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        // Describe rotation around the Z axis.
        glm::mat4 rotationMatrix_Z = glm::rotate(
            identity, 
            glm::radians(rotationDegrees.z), 
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        
        // reads from right to left, rotation needs to be added X, then Y, then Z.
        glm::mat4 rotationMatrix = rotationMatrix_Z * rotationMatrix_Y * rotationMatrix_X; 
        
        // Describe the object's position.
        glm::mat4 translationMatrix = glm::translate(
            identity, 
            position
        );
        
        
        // reads from right to left, applying scale, then rotation, then position.           y=C(B(Ax)) 
        glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        
        return modelMatrix;
    }
    
    glm::mat4 BuildModelMatrix(const AcornData::Transform& transform)
    {
        return BuildModelMatrix(
            transform.position,
            transform.scale,
            transform.rotationDegrees
        );
    }
    
    
}
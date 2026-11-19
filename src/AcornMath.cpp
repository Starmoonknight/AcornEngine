#include "AcornMath.h"

// Standard library
#include <cassert>
#include <cmath>

// Third-party libraries
#include <glm/gtc/matrix_transform.hpp>         // Model, view, and projection transforms
          

namespace
{
    
    // Returns true when every component is a finite number.
    // Rejects infinity and NaN ("not a number").
    bool IsFinite(const glm::vec3& value)
    {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    // Returns a unit-length quaternion suitable for rotation calculations.
    // Requires a finite, nonzero quaternion with a representable squared length.
    glm::quat Normalized(const glm::quat& value)
    {
        // A quaternion dotted with itself gives its squared length.
        const float lengthSquared = glm::dot(value, value);
        
        // Catch invalid input in builds where assertions are enabled.
        assert(std::isfinite(lengthSquared) && lengthSquared > 0.0f);
        return glm::normalize(value);
    }

    // Returns a unit-length vector pointing in the same direction.
    // Requires a finite, nonzero vector with a representable squared length.
    // A zero vector has no direction and cannot be normalized.
    glm::vec3 UnitDirection(const glm::vec3& value)
    {
        const float lengthSquared = glm::dot(value, value);
        
        // Catch non-finite components and zero length in assertion-enabled builds.
        assert(IsFinite(value) && lengthSquared > 0.0f);
        
        return glm::normalize(value);
    }
    
}



namespace AcornMath
{
    
    glm::quat MakeOrientationXYZ(const glm::vec3& rotationDegrees)
    {
        assert(IsFinite(rotationDegrees));
        const glm::vec3 radians = glm::radians(rotationDegrees);

        const glm::quat rotationX = glm::angleAxis(radians.x, glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::quat rotationY = glm::angleAxis(radians.y, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::quat rotationZ = glm::angleAxis(radians.z, glm::vec3(0.0f, 0.0f, 1.0f));
        // Rightmost rotation acts first: X, then Y, then Z.
        return glm::normalize(rotationZ * rotationY * rotationX);
    }

    glm::quat MakeOrientationXYZ(const AcornData::EulerAnglesDegrees& angles)
    {
        return MakeOrientationXYZ(glm::vec3(angles.pitch, angles.yaw, angles.roll));
    }

    glm::vec3 GetRotationXYZDegrees(const glm::quat& orientation)
    {
        // Requires a valid, nonzero quaternion.
        const glm::quat normalizedOrientation = glm::normalize(orientation);
        
        return glm::degrees(glm::eulerAngles(normalizedOrientation));
    }

    
    
    glm::vec3 GetWorldDirection(const glm::quat& orientation, const glm::vec3& localDirection)
    {
        assert(std::abs(glm::dot(orientation, orientation) - 1.0f) < 0.001f);
        assert(IsFinite(localDirection));
        
        return orientation * localDirection;
    }
    
    

    glm::vec3 GetForwardDirection(const glm::quat& orientation, glm::vec3 localForward)
    {
        return GetWorldDirection(orientation, localForward);
    }

    glm::vec3 GetForwardDirection(const glm::vec3& rotationDegrees, glm::vec3 localForward)
    {
        return GetForwardDirection(MakeOrientationXYZ(rotationDegrees), localForward);
    }

    
    glm::vec3 GetRightDirection(const glm::quat& orientation, glm::vec3 localRight)
    {
        return GetWorldDirection(orientation, localRight);
    }

    glm::vec3 GetRightDirection(const glm::vec3& rotationDegrees, glm::vec3 localRight)
    {
        return GetRightDirection(MakeOrientationXYZ(rotationDegrees), localRight);
    }
    
    
    glm::vec3 GetUpDirection(const glm::quat& orientation, glm::vec3 localUp)
    {
        return GetWorldDirection(orientation, localUp);
    }
    
    glm::vec3 GetUpDirection(const glm::vec3& rotationDegrees, glm::vec3 localUp)
    {
        return GetUpDirection(MakeOrientationXYZ(rotationDegrees), localUp);
    }


    glm::vec3 GetRightDirection_FromForward(const glm::vec3& forward, glm::vec3 upReference)
    {
        return UnitDirection(glm::cross(UnitDirection(forward), UnitDirection(upReference)));
    }
    
    glm::vec3 GetUpDirection_FromForwardRight(const glm::vec3& forward, const glm::vec3& right)
    {
        return UnitDirection(glm::cross(UnitDirection(right), UnitDirection(forward)));;
    }
    
    glm::vec3 GetDirectionTo(const glm::vec3& position, const glm::vec3& target)
    {
        assert(position != target);
        
        return UnitDirection(target - position);
    }

    bool TryMakeLookOrientation(const glm::vec3& position, const glm::vec3& target, 
        glm::quat& result, glm::vec3 upReference)
    {
        if (!IsFinite(position) || !IsFinite(target) || !IsFinite(upReference))
        {
            return false;            
        }
        
        const glm::vec3 offset = target - position;
        const float distanceSquared = glm::dot(offset, offset);
        const float upLengthSquared = glm::dot(upReference, upReference);
        if (!std::isfinite(distanceSquared) || distanceSquared <= 0.0f ||
            !std::isfinite(upLengthSquared) || upLengthSquared <= 0.0f)
        {            
            return false;
        }
        
        const glm::vec3 forward = glm::normalize(offset);
        const glm::vec3 up = glm::normalize(upReference);
        const glm::vec3 side = glm::cross(forward, up);
        if (glm::dot(side, side) < 0.000001f)
        {            
            return false;
        }
        
        result = Normalized(glm::quatLookAtRH(forward, up));
        return true;        
    }
    
    
    
    void RotateLocal(glm::quat& orientation, const glm::quat& change)
    {
        const glm::quat current = Normalized(orientation);
        const glm::quat delta = Normalized(change);
        orientation = Normalized(current * delta);
    }

    void RotateLocal(glm::quat& orientation, const glm::vec3& changeDegrees)
    {
        RotateLocal(orientation, MakeOrientationXYZ(changeDegrees));
    }

    void RotateLocal(glm::quat& orientation, float angleDegrees, const glm::vec3& unitAxis)
    {
        assert(std::isfinite(angleDegrees) && IsFinite(unitAxis));
        assert(std::abs(glm::dot(unitAxis, unitAxis) - 1.0f) < 0.001f);
        RotateLocal(orientation, glm::angleAxis(glm::radians(angleDegrees), unitAxis));
    }
    
    void RotateLocal(glm::vec3& rotationDegrees, const glm::vec3& changeDegrees)
    {
        glm::quat orientation = MakeOrientationXYZ(rotationDegrees);
        RotateLocal(orientation, changeDegrees);
        rotationDegrees = GetRotationXYZDegrees(orientation);
    }

    
    
    void RotateWorld(glm::quat& orientation, const glm::quat& change)
    {
        const glm::quat current = Normalized(orientation);
        const glm::quat delta = Normalized(change);
        orientation = Normalized(delta * current);
    }

    void RotateWorld(glm::quat& orientation, const glm::vec3& changeDegrees)
    {
        RotateWorld(orientation, MakeOrientationXYZ(changeDegrees));
    }

    void RotateWorld(glm::quat& orientation, float angleDegrees, const glm::vec3& unitAxis)
    {
        assert(std::isfinite(angleDegrees) && IsFinite(unitAxis));
        assert(std::abs(glm::dot(unitAxis, unitAxis) - 1.0f) < 0.001f);
        RotateWorld(orientation, glm::angleAxis(glm::radians(angleDegrees), unitAxis));
    }

    void RotateWorld(glm::vec3& rotationDegrees, const glm::vec3& changeDegrees)
    {
        glm::quat orientation = MakeOrientationXYZ(rotationDegrees);
        RotateWorld(orientation, changeDegrees);
        rotationDegrees = GetRotationXYZDegrees(orientation);
    }

    
    
    void TranslateWorld(glm::vec3& position, const glm::vec3& displacement)
    {
        position += displacement;
    }

    void TranslateLocal(glm::vec3& position, const glm::quat& orientation, const glm::vec3& displacement)
    {
        position += GetWorldDirection(orientation, displacement);
    }
    
    void TranslateLocal(glm::vec3& position, const glm::vec3& rotationDegrees, const glm::vec3& displacement)
    {
        TranslateLocal(position, MakeOrientationXYZ(rotationDegrees), displacement);
    }
    
    
    
    void ScaleBy(glm::vec3& scale, const glm::vec3& factors)
    {
        scale *= factors;
    }

    
    
    // ----- ----- Matrix Math ----- -----
    
    glm::mat4 BuildRotationMatrixXYZ(const glm::vec3& rotationDegrees)
    {
        return BuildRotationMatrix(MakeOrientationXYZ(rotationDegrees));
    }
    
    glm::mat4 BuildRotationMatrix(const glm::quat& orientation)
    {
        return glm::mat4_cast(Normalized(orientation));
    }
    
    

    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees)
    {
        return BuildModelMatrix(position, scale, MakeOrientationXYZ(rotationDegrees));
        
        /*  Old model matrix rotation building part: 
         
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
        */
        
    }
    
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, const glm::quat& orientation)
    {
        // A starting matrix that makes no change.
        const glm::mat4 identity(1.0f);
        
        // Describe the object's size.
        const glm::mat4 scaleMatrix = glm::scale(
            identity, 
            scale
        );
        
        const glm::mat4 rotationMatrix = BuildRotationMatrix(orientation);
        
        // Describe the object's position.
        const glm::mat4 translationMatrix = glm::translate(
            identity, 
            position
        );
        
        
        // reads from right to left, applying scale, then rotation, then position.           y=C(B(Ax)) 
        const glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        
        return modelMatrix;
    }

    glm::mat4 BuildModelMatrix(const AcornData::Transform& transform)
    {
        return BuildModelMatrix(transform.position, transform.scale, transform.rotationDegrees);
    }
    
    
    
}

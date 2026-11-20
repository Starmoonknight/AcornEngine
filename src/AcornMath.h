#pragma once


// Third-party libraries
#include <glm/glm.hpp>          // Vectors, matrices, dot and cross products
#include <glm/gtc/quaternion.hpp>   



namespace AcornData
{
    
    // Named input for the model's XYZ Euler rotation convention, measured in degrees.
    // pitch maps to X, yaw to Y, and roll to Z.
    // These names do not change rotation signs or apply camera inversion.
    // For a -Z-forward camera, positive yaw turns left from the unrotated pose.
    struct EulerAnglesDegrees
    {
        float pitch = 0.0f;
        float yaw = 0.0f;
        float roll = 0.0f;
    };   
    
    struct Transform
    {
        glm::vec3 position{0.0f};
        glm::vec3 rotationDegrees{0.0f};
        glm::vec3 scale{1.0f};
    };
    
}




namespace AcornMath
{
    
    // Converts an absolute XYZ rotation in degrees into a unit quaternion.
    // Matches the original model matrix: X acts first, then Y, then Z.
    // The quaternion describes orientation, not a count of completed revolutions.
    // Requires finite angle values.
    glm::quat MakeOrientationXYZ(const glm::vec3& rotationDegrees);
    
    // Converts named Euler angles using pitch=X, yaw=Y, roll=Z.
    // Uses the same signs and rotation order as the vec3 overload.
    // Positive yaw follows positive-Y rotation, not a positive-right input convention.
    glm::quat MakeOrientationXYZ(const AcornData::EulerAnglesDegrees& angles);
    
    // Describes a quaternion orientation as XYZ Euler angles in degrees.
    // Normalizes the input before extracting angles.
    // Returned values need not match the original angles and may jump during smooth motion.
    // Euler singularities make the description ambiguous and numerically sensitive.
    // Requires a valid, nonzero quaternion. Prefer quaternion storage for ongoing rotation.
    glm::vec3 GetRotationXYZDegrees(const glm::quat& orientation);
    
    
    // Returns an orientation between start and target along the shortest rotation route.
    // progress=0 gives start; progress=1 gives target. Values outside this range are clamped.
    // Normalizes both endpoints and the result.
    // Requires valid, nonzero quaternions and finite progress.
    // Does not store time, modify the endpoints, or preserve extra full revolutions.
    glm::quat InterpolateOrientation(
        const glm::quat& start,
        const glm::quat& target,
        float progress);
    
    
    
    
    // --------- --------- Direction --------- ---------  
    
    // Rotates a vector from local coordinates into world coordinates.
    // Uses orientation to determine where the local axes point in the world.
    // Preserves vector length; does not add position or apply scale.
    // Requires a unit quaternion and a finite localVector.
    // Suitable for directions and movement offsets, but not a complete point transform.
    glm::vec3 RotateLocalVectorToWorld(const glm::quat& orientation, const glm::vec3& localDirection);
    
    // Returns the chosen local forward vector rotated into world coordinates.
    // Requires a unit quaternion. The default forward vector is local -Z.
    // Preserves the supplied vector's length.
    glm::vec3 GetForwardDirection(const glm::quat& orientation, glm::vec3 localForward = glm::vec3(0.0f, 0.0f, -1.0f));
    // Returns forward using an XYZ Euler orientation in degrees.
    // Converts the orientation to a quaternion, then rotates localForward.
    // Preserves the supplied vector's length.
    glm::vec3 GetForwardDirection(const glm::vec3& rotationDegrees, glm::vec3 localForward = glm::vec3(0.0f, 0.0f, -1.0f));
    
    // Returns the chosen local right vector rotated into world coordinates.
    // Requires a unit quaternion. The default right vector is local +X.
    // Includes roll and preserves the supplied vector's length.
    glm::vec3 GetRightDirection(const glm::quat& orientation, glm::vec3 localRight = glm::vec3(1.0f, 0.0f, 0.0f));
    // Returns right using an XYZ Euler orientation in degrees.
    // Converts the orientation to a quaternion, then rotates localRight.
    // Includes roll and preserves the supplied vector's length.
    glm::vec3 GetRightDirection(const glm::vec3& rotationDegrees,  glm::vec3 localRight = glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Returns the chosen local up vector rotated into world coordinates.
    // Requires a unit quaternion. The default up vector is local +Y.
    // Includes roll and preserves the supplied vector's length.
    glm::vec3 GetUpDirection(const glm::quat& orientation, glm::vec3 localUp = glm::vec3(0.0f, 1.0f, 0.0f));
    // Returns up using an XYZ Euler orientation in degrees.
    // Converts the orientation to a quaternion, then rotates localUp.
    // Includes roll and preserves the supplied vector's length.
    glm::vec3 GetUpDirection(const glm::vec3& rotationDegrees, glm::vec3 localUp = glm::vec3(0.0f, 1.0f, 0.0f));
    
    
    
    // Constructs a unit right direction from world-space forward and an up reference.
    // Both inputs must be finite, nonzero, and not parallel or opposite.
    // The up reference chooses the roll; forward alone cannot recover an existing roll.
    // Inputs must also have representable, nonzero squared lengths.
    glm::vec3 GetRightDirection_FromForward(const glm::vec3& forward, glm::vec3 upReference = glm::vec3(0.0f, 1.0f, 0.0f));
    // Constructs a unit up direction perpendicular to world-space forward and right.
    // Both inputs must be finite, nonzero, and not parallel or opposite.
    // The supplied right direction determines the roll around forward.
    // Inputs must also have representable, nonzero squared lengths.
    glm::vec3 GetUpDirection_FromForwardRight( const glm::vec3& forward, const glm::vec3& right);
    
    // Returns a unit world-space direction from position toward target.
    // Does not change either position or any orientation.
    // Requires distinct finite positions whose difference has a finite,
    // nonzero squared length.
    glm::vec3 GetDirectionTo( const glm::vec3& position, const glm::vec3& target);
    // Builds a unit orientation whose local -Z faces the world-space target.
    // Uses upReference to choose roll, adjusting it to be perpendicular to forward.
    // Returns false without changing result for invalid inputs, coincident positions,
    // or a viewing direction nearly parallel or opposite to upReference.
    // Does not move anything or remember the target.
    bool TryMakeLookOrientation(const glm::vec3& position, const glm::vec3& target,
                                glm::quat& result, glm::vec3 upReference = glm::vec3(0, 1, 0));
    
    
    // --------- --------- Rotation --------- ---------  
    
    // Adds a turn toward the object's own right, measured in degrees.
    // Assumes local forward is -Z and local up is +Y.
    // Positive degrees means right; internally uses negative local-Y rotation.
    // Follows the object's current orientation, including roll.
    // Modifies orientation only and does not read camera inversion settings.
    void YawLocal(glm::quat& orientation, float degrees);

    // Rotates a local-space vector into world coordinates.
    // Requires a unit quaternion and a finite localDirection.
    // Preserves vector length; does not automatically make the result unit length.
    // Applies rotation only, with no position or scale.S
    void YawLocal(glm::vec3& rotationDegrees, float degrees);
    
    
    // Adds a quaternion turn in the object's local coordinate frame.
    // Modifies orientation; no position is involved.
    // Normalizes both inputs and the result.
    // Requires valid, nonzero quaternions with finite, nonzero squared lengths.
    void RotateLocal(glm::quat& orientation, const glm::quat& change);
    // Adds an XYZ-composed local turn, measured in degrees.
    // Keeps orientation stored as a quaternion.
    // This combines rotations rather than adding Euler-angle components.
    // A combined XYZ change differs from separate local X, then Y, then Z calls.
    void RotateLocal(glm::quat& orientation, const glm::vec3& changeDegrees);
    // Adds a turn around one axis expressed in the object's local coordinates.
    // angleDegrees must be finite; unitAxis must be finite and unit length.
    // Modifies orientation while keeping quaternion storage.
    void RotateLocal(glm::quat& orientation, float angleDegrees, const glm::vec3& unitAxis);
    // Adds an XYZ-composed local turn to an orientation stored as Euler degrees.
    // Converts to a quaternion, applies the turn, then writes Euler angles back.
    // Returned angle values can jump and retain Euler extraction limitations.
    // Prefer the quaternion overload for unrestricted camera rotation.
    void RotateLocal(glm::vec3& rotationDegrees, const glm::vec3& changeDegrees);
    
    // Adds a quaternion turn expressed in the fixed world coordinate frame.
    // Modifies orientation only; it does not orbit an object around the world origin.
    // Normalizes both inputs and the result.
    // Requires valid, nonzero quaternions with finite, nonzero squared lengths.
    void RotateWorld(glm::quat& orientation, const glm::quat& change);
    // Adds an XYZ-composed world-space turn, measured in degrees.
    // Uses fixed world axes rather than the object's rotated local axes.
    // Keeps orientation stored as a quaternion.
    void RotateWorld(glm::quat& orientation, const glm::vec3& changeDegrees);
    // Adds a turn around one axis expressed in world coordinates.
    // angleDegrees must be finite; unitAxis must be finite and unit length.
    // Modifies orientation only; no position is involved.
    void RotateWorld(glm::quat& orientation, float angleDegrees, const glm::vec3& unitAxis);
    // Adds an XYZ-composed world-space turn to an orientation stored as Euler degrees.
    // Converts to a quaternion, applies the turn, then writes Euler angles back.
    // Returned angle values can jump and retain Euler extraction limitations.
    // Prefer the quaternion overload for unrestricted camera rotation.
    void RotateWorld(glm::vec3& rotationDegrees, const glm::vec3& changeDegrees);
    
    
    // --------- --------- Translation --------- ---------  
    
    // Adds displacement directly to a world-space position.
    // Displacement is measured along fixed world axes, in world units.
    // Supply finite position and displacement values.
    void TranslateWorld(glm::vec3& position, const glm::vec3& displacement);
    // Adds local-space displacement to a world-space position.
    // Rotates the displacement using orientation; requires a unit quaternion.
    // With the camera's default basis, negative local Z moves forward.
    // Ignores model scale and does not change orientation.
    void TranslateLocal(glm::vec3& position, const glm::quat& orientation, const glm::vec3& displacement);
    // Adds local-space displacement using an XYZ Euler orientation in degrees.
    // Converts the orientation to a quaternion before calculating movement.
    // Does not modify rotationDegrees and ignores model scale.
    void TranslateLocal(glm::vec3& position, const glm::vec3& rotationDegrees, const glm::vec3& displacement);
    
    
    // --------- --------- Scale --------- ---------  
    
    // Multiplies each existing scale component by its matching factor.
    // For example, a factor of 2 doubles that component; 1 leaves it unchanged.
    // Does not modify position or rotation.
    void ScaleBy(glm::vec3& scale, const glm::vec3& factors);
    
    
    
    
    
    // ----- ----- Matrix Math ----- -----
    
    // Builds a rotation-only matrix from XYZ Euler angles in degrees.
    // Matches the original model rotation order: X acts first, then Y, then Z.
    // Contains no translation or scaling.
    glm::mat4 BuildRotationMatrixXYZ(const glm::vec3& rotationDegrees);
    // Builds a rotation-only matrix from a quaternion.
    // Normalizes the quaternion before building the matrix.
    // Requires a valid, nonzero quaternion with a finite, nonzero squared length.
    // Contains no translation or scaling.
    glm::mat4 BuildRotationMatrix(const glm::quat& orientation);
    
    // Builds a model-to-world matrix using XYZ Euler angles in degrees.
    // Each vertex is scaled, then rotated around X, Y, and Z, then translated.
    // Preserves the original argument order: position, scale, rotation.
    // Supply finite values.
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees);
    // Builds a model-to-world matrix using a quaternion orientation.
    // Each vertex is scaled, then rotated, then translated.
    // Normalizes orientation; requires a valid, nonzero quaternion.
    // Preserves the argument order: position, scale, orientation.
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, const glm::quat& orientation);
    // Builds a model-to-world matrix from the supplied Transform.
    // Uses its position, scale, and XYZ rotationDegrees.
    // Does not modify the Transform.
    glm::mat4 BuildModelMatrix(const AcornData::Transform& transform);
    
    

    
}


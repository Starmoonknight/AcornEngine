#pragma once


// Third-party libraries
#include <glm/glm.hpp>          // Vectors, matrices, dot and cross products
#include <glm/gtc/quaternion.hpp>   



namespace AcornData
{
    
    // Euler input fields map to X=pitch, Y=yaw, Z=roll.
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
    
    // Convention: positive X/Y/Z axes; composition Rz * Ry * Rx.
    glm::quat MakeOrientationXYZ(const glm::vec3& rotationDegrees);
    
    // Maps named fields explicitly; never relies on their declaration order.
    glm::quat MakeOrientationXYZ(const AcornData::EulerAnglesDegrees& angles);
    
    // Returns equivalent angles, not necessarily the original angle values.
    // Reading these angles is safe; accumulating them retains Euler limitations.
    glm::vec3 GetRotationXYZDegrees(const glm::quat& orientation);
    
    
    
    // Adds rightward yaw for an orientation whose local forward is -Z and up is +Y.
    void YawLocal(glm::quat& orientation, float degrees);

    // Same operation for existing XYZ Euler storage; retains Euler limitations.
    void YawLocal(glm::vec3& rotationDegrees, float degrees);
    
    
    
    
    // --------- --------- Direction --------- ---------  
    
    // Rotates a LOCAL vector into WORLD space. Requires a unit quaternion.
    // Vector length is preserved: a unit input yields a unit output.
    // A world-space vector must not be passed as though it were local.
    glm::vec3 GetWorldDirection(const glm::quat& orientation, const glm::vec3& localDirection);
    
    // Returns world-space forward after rotating the chosen local axis.
    // Quaternion input must be unit length; Euler input uses the XYZ convention.
    glm::vec3 GetForwardDirection(const glm::quat& orientation, glm::vec3 localForward = glm::vec3(0.0f, 0.0f, -1.0f));
    glm::vec3 GetForwardDirection(const glm::vec3& rotationDegrees, glm::vec3 localForward = glm::vec3(0.0f, 0.0f, -1.0f));
    
    // Returns world-space right after rotating the chosen local axis.
    // Quaternion input must be unit length; Euler input uses the XYZ convention.
    glm::vec3 GetRightDirection(const glm::quat& orientation, glm::vec3 localRight = glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec3 GetRightDirection(const glm::vec3& rotationDegrees,  glm::vec3 localRight = glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Returns world-space up after rotating the chosen local axis.
    // Quaternion input must be unit length; Euler input uses the XYZ convention.
    glm::vec3 GetUpDirection(const glm::quat& orientation, glm::vec3 localUp = glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 GetUpDirection(const glm::vec3& rotationDegrees, glm::vec3 localUp = glm::vec3(0.0f, 1.0f, 0.0f));
    
    
    
    // Constructs world-space right from world-space forward and an up reference.
    // Inputs must be finite, nonzero and nonparallel. Does not recover unknown roll.
    glm::vec3 GetRightDirection_FromForward(const glm::vec3& forward, glm::vec3 upReference = glm::vec3(0.0f, 1.0f, 0.0f));
    // Constructs world-space up from world-space forward and right, including their roll.
    // Inputs must be finite, nonzero and nonparallel.
    glm::vec3 GetUpDirection_FromForwardRight( const glm::vec3& forward, const glm::vec3& right);
    // Returns a unit direction toward target. Requires distinct finite positions. Requires position and target to be different.
    glm::vec3 GetDirectionTo( const glm::vec3& position, const glm::vec3& target);
    // Builds a right-handed orientation facing target with the supplied up reference.
    // Returns false without changing result for invalid/coincident/parallel inputs.
    bool TryMakeLookOrientation(const glm::vec3& position, const glm::vec3& target,
                                glm::quat& result, glm::vec3 upReference = glm::vec3(0, 1, 0));
    
    
    // --------- --------- Rotation --------- ---------  
    
    // Adds rotation around the current local axes, without moving position.
    // Quaternion changes must be finite and nonzero; they are normalized internally.
    void RotateLocal(glm::quat& orientation, const glm::quat& change);
    // Adds an XYZ-composed change expressed in degrees, keeping quaternion storage.
    void RotateLocal(glm::quat& orientation, const glm::vec3& changeDegrees);
    // Adds one axis-angle rotation. unitAxis is expressed in local space.
    void RotateLocal(glm::quat& orientation, float angleDegrees, const glm::vec3& unitAxis);
    // Compatibility for Euler storage; returned angles can jump at singularities.
    // Use quaternion storage for a freely rolling camera.
    void RotateLocal(glm::vec3& rotationDegrees, const glm::vec3& changeDegrees);
    
    // Adds rotation around world-aligned axes, without moving position.
    // Quaternion changes must be finite and nonzero; they are normalized internally.
    void RotateWorld(glm::quat& orientation, const glm::quat& change);
    // Adds an XYZ-composed change expressed in degrees, keeping quaternion storage.
    void RotateWorld(glm::quat& orientation, const glm::vec3& changeDegrees);
    // Adds one axis-angle rotation. unitAxis is expressed in world space.
    void RotateWorld(glm::quat& orientation, float angleDegrees, const glm::vec3& unitAxis);
    // Compatibility for Euler storage; returned angles can jump at singularities.
    // Use quaternion storage for a freely rolling camera.
    void RotateWorld(glm::vec3& rotationDegrees, const glm::vec3& changeDegrees);
    
    
    // --------- --------- Translation --------- ---------  
    
    // Adds a displacement measured along fixed world axes.
    void TranslateWorld(glm::vec3& position, const glm::vec3& displacement);
    // Adds displacement along the object's rotated axes; requires a unit orientation.
    // With the default camera basis, local negative Z is forward. Scale is ignored.
    void TranslateLocal(glm::vec3& position, const glm::quat& orientation, const glm::vec3& displacement);
    // Same movement using an XYZ Euler orientation in degrees.
    void TranslateLocal(glm::vec3& position, const glm::vec3& rotationDegrees, const glm::vec3& displacement);
    
    
    // --------- --------- Scale --------- ---------  
    
    // Multiplies each scale component by its matching factor; does not change position.
    void ScaleBy(glm::vec3& scale, const glm::vec3& factors);
    
    
    
    
    
    // ----- ----- Matrix Math ----- -----
    
    // Builds a pure rotation matrix matching the XYZ conversion (no position/scale).
    glm::mat4 BuildRotationMatrixXYZ(const glm::vec3& rotationDegrees);
    // Builds a pure rotation matrix from a valid, nonzero quaternion.
    glm::mat4 BuildRotationMatrix(const glm::quat& orientation);
    
    // Builds T * R * S: scale, then rotate, then translate each vertex.
    // Parameter order retains the original API: position, scale, rotation.
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationDegrees);
    // Same model transform with quaternion orientation, including camera-derived rotations.
    glm::mat4 BuildModelMatrix(glm::vec3 position, glm::vec3 scale, const glm::quat& orientation);
    // Uses the position, scale and Euler rotation stored in Transform.
    glm::mat4 BuildModelMatrix(const AcornData::Transform& transform);
    
    

    
}


#pragma once

#include "AcornMath.h"

// Third-party libraries
#include <glm/glm.hpp>                          // Vectors, matrices, dot and cross products



// Note: later doublecheck what namespace is best to have things in, math-helpers, data, etc.
namespace Rendering
{
    
    
    enum class ProjectionType
    {
        PERSPECTIVE,
        ORTHOGRAPHIC,
    };
    
    // is this best to have on camera or arm? 
    enum class CamereaMovementMode
    {
        FreeFlight,
        FirstPerson_Follow,
        ThirdPerson_Orbit,
        FixedPosition,
    };
    
    // Rectangle in framebuffer pixels, not logical window coordinates.
    // The renderer must also apply this rectangle with glViewport.
    struct Viewport
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };
    
    struct ProjectionSettings
    {
        ProjectionType projectionType = ProjectionType::PERSPECTIVE;
        
        float verticalFovDegrees = 70.0f;
        float orthographicHeight = 10.0f;

        float nearPlane = 0.1f;
        float farPlane = 100.0f;
    };
    
    
    // Preferences applied only to look input, not to XYZ rotation setters/getters.
    struct LookControlSettings
    {
        bool invertHorizontal = false;
        bool invertVertical = false;
    };
    
    
    // Pose and projection data; movement controllers can modify it through this API.
    // Forward=-Z, right=+X, up=+Y before rotation. Keep orientation unit length.
    // Derived directions and matrices are calculated on request, not cached.
    struct CameraData
    {
        glm::vec3 position {0.0f, 0.0f, 5.0f};
        glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
        ProjectionSettings projectionSettings; 
        LookControlSettings lookControls;
        Viewport viewport;
        
        
        // Maybe add back later, but should probably be in the arm:
        //glm::vec3 cameraTarget = glm::vec3(0.0f);
        
        
        // Default pose; viewport has no drawable area until configured.
        CameraData() = default;
        // Initial position and optional XYZ rotation in degrees.
        explicit CameraData(const glm::vec3& startingPosition, const glm::vec3& startingRotationDegrees = glm::vec3(0.0f));
        // Same setup with named angles, mapped as pitch=X, yaw=Y, roll=Z.
        CameraData(const glm::vec3& startingPosition, const AcornData::EulerAnglesDegrees& startingAngles);
        // Same setup using a valid, nonzero quaternion, normalized internally.
        CameraData(const glm::vec3& startingPosition, const glm::quat& startingOrientation);    
        
        
        // Returns this camera's own forward direction, expressed in world coordinates.
        // Rotates its local forward axis (-Z) by its current orientation.
        // Changes when the camera rotates; does not change when it moves.
        glm::vec3 GetForward() const;
        
        // Returns this camera's own right direction, expressed in world coordinates.
        // Rotates its local right axis (+X) by its current orientation, including roll.
        glm::vec3 GetRight() const;
        
        // Returns this camera's own up direction, expressed in world coordinates.
        // Rotates its local up axis (+Y) by its current orientation, including roll.
        // This can differ from the fixed world-up direction (0, 1, 0).
        glm::vec3 GetUp() const;
        
        // Returns an XYZ Euler-angle description of the camera's orientation
        // relative to the world axes, in degrees, using Rz * Ry * Rx.
        // This is an orientation, not a direction vector.
        // Equivalent orientations can produce different angle values.
        glm::vec3 GetRotationDegrees() const;
        
        
        // Replaces rotation using positive-axis Rz * Ry * Rx Euler degrees.
        void SetRotationDegrees(const glm::vec3& rotationDegrees);
        // Same replacement using named angle fields.
        void SetRotationDegrees(const AcornData::EulerAnglesDegrees& angles);
        // Replaces orientation; requires a finite, nonzero quaternion.
        void SetOrientation(const glm::quat& newOrientation);
        // Replaces position; this is not an additional displacement.
        void SetPosition(const glm::vec3& newPosition);
        
        // Aims at a target using an up reference, choosing roll to match that reference.
        // Returns false and preserves orientation for invalid/degenerate inputs.
        // Call repeatedly to track a target; this does not move/orbit the camera.
        bool LookAt(const glm::vec3& target, glm::vec3 upReference = glm::vec3(0, 1, 0));
        
        
        // Adds a look change in degrees: X=rightward yaw, Y=upward pitch.
        // Uses camera-local axes, including roll; horizontal yaw is applied first.
        // Applies lookControls inversion preferences. Does not move the camera,
        // reset roll, clamp pitch, or reinterpret the XYZ rotation methods.
        void ApplyLookDelta(const glm::vec2& lookDegrees);
        
        
        // ---- Local Transform ----
        
        // Adds rotation around local axes; position is unchanged.
        void RotateLocal(const glm::vec3& changeDegrees);
        // Same operation with an additional quaternion rotation.
        void RotateLocal(const glm::quat& change);
        // Adds displacement along local axes; local -Z is camera forward.
        void TranslateLocal(const glm::vec3& displacement);
        
        // Convenience wrapper: adds rotation about local positive X only.
        void RotateLocalX(float angleDegrees);
        // Convenience wrapper: moves along local X only; distance may be negative.
        void TranslateLocalX(float distance);
        // Convenience wrapper: adds rotation about local positive Y only.
        void RotateLocalY(float angleDegrees);
        // Convenience wrapper: moves along local Y only; distance may be negative.
        void TranslateLocalY(float distance);
        // Convenience wrapper: adds rotation about local positive Z only.
        void RotateLocalZ(float angleDegrees);
        // Convenience wrapper: moves along local Z only; distance may be negative.
        void TranslateLocalZ(float distance);
        
        
        // ---- World Transform ----
        
        // Adds rotation around world axes; position is unchanged.
        void RotateWorld(const glm::vec3& changeDegrees);
        // Same operation with an additional quaternion rotation.
        void RotateWorld(const glm::quat& change);
        // Adds displacement along world axes; local -Z is camera forward.
        void TranslateWorld(const glm::vec3& displacement);
        
        // Convenience wrapper: adds rotation about world positive X only.
        void RotateWorldX(float angleDegrees);
        // Convenience wrapper: moves along world X only; distance may be negative.
        void TranslateWorldX(float distance);
        // Convenience wrapper: adds rotation about world positive Y only.
        void RotateWorldY(float angleDegrees);
        // Convenience wrapper: moves along world Y only; distance may be negative.
        void TranslateWorldY(float distance);
        // Convenience wrapper: adds rotation about world positive Z only.
        void RotateWorldZ(float angleDegrees);
        // Convenience wrapper: moves along world Z only; distance may be negative.
        void TranslateWorldZ(float distance);
        
        
        // ---- Info ----
        
        // Stores a framebuffer rectangle. Zero dimensions are allowed while minimized.
        void SetViewport(int x, int y, int width, int height);
        // Check before requesting a projection matrix or drawing.
        bool HasValidViewport() const;
        // Computes width/height; requires a valid viewport. No duplicate aspect state.
        float GetAspectRatio() const;
        // Returns world-to-camera transform using current orientation, including roll.
        glm::mat4 GetViewMatrix() const;
        // Builds projection from current settings and viewport aspect ratio.
        glm::mat4 GetProjectionMatrix() const;
        
        
    };
    
    
    
    // Free-function equivalents of the camera's matrix getters.
    glm::mat4 BuildViewMatrix(const CameraData& camera);
    
    
    // Uses standard right-handed OpenGL depth [-1,+1], independent of GLM default macros.
    // Requires aspect>0, 0<near<far, 0<FOV<180, and positive orthographic height.
    glm::mat4 BuildProjectionMatrix(const ProjectionSettings& settings, float aspectRatio);
    
}


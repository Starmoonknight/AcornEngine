#pragma once


// Project headers
#include "AcornMath.h"



// Third-party libraries
#include <glm/glm.hpp>                          // Vectors, matrices, dot and cross products



// Note: later doublecheck what namespace is best to have things in, math-helpers, data, etc.
namespace Rendering
{
    
    struct CameraMatrices
    {
        glm::mat4 view;
        glm::mat4 projection;
    };
    
    
    // is this best to have on camera or arm? 
    // Currently not in use
    enum class CamereaMovementMode
    {
        FreeFlight,
        FirstPerson_Follow,
        ThirdPerson_Orbit,
        FixedPosition,
    };
    
    
    enum class ProjectionType
    {
        PERSPECTIVE,
        ORTHOGRAPHIC,
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
        
        
        // Creates a camera at (0, 0, 5), facing world -Z, with world +Y as up.
        // Uses the default projection and look-control settings.
        // The viewport has zero area until configure
        CameraData() = default;
        // Creates a camera at startingPosition with an absolute XYZ rotation in degrees.
        // Uses the same rotation order as the model matrix: X, then Y, then Z.
        // Omitting rotation gives the unrotated orientation: forward -Z and up +Y.
        // Does not configure the viewport.
        explicit CameraData(const glm::vec3& startingPosition, const glm::vec3& startingRotationDegrees = glm::vec3(0.0f));
        // Creates a camera using named Euler angles: pitch=X, yaw=Y, roll=Z.
        // Uses the same signs and rotation order as the vec3 constructor.
        // Positive yaw is positive-Y rotation; from the unrotated pose, it turns left.
        // Does not configure the viewport.
        CameraData(const glm::vec3& startingPosition, const AcornData::EulerAnglesDegrees& startingAngles);
        // Creates a camera using a quaternion orientation.
        // Normalizes the quaternion without intentionally changing its orientation.
        // Requires a finite, nonzero quaternion with a finite, nonzero squared length.
        // Does not configure the viewport.
        CameraData(const glm::vec3& startingPosition, const glm::quat& startingOrientation);    
        
        
        // Returns the camera's forward direction in world coordinates.
        // Includes all camera rotation, including pitch, yaw, and roll.
        // With no rotation, forward is (0, 0, -1).
        // Requires the stored orientation to be a unit quaternion.
        glm::vec3 GetForward() const;
        
        // Returns the camera's own right direction in world coordinates.
        // This direction follows the camera when it turns or rolls.
        // With no rotation, right is (1, 0, 0).
        // Requires the stored orientation to be a unit quaternion.
        glm::vec3 GetRight() const;
        
        // Returns the camera's own up direction in world coordinates.
        // This follows camera roll and can differ from world up.
        // With no rotation, up is (0, 1, 0).
        // Requires the stored orientation to be a unit quaternion.
        glm::vec3 GetUp() const;
        
        // Describes the current orientation as XYZ Euler angles in degrees.
        // Uses the same rotation convention as SetRotationDegrees.
        // Values need not match the original input or stay within 0..360.
        // Values can jump during smooth rotation and are ambiguous near Euler singularities.
        // Use for inspection; keep quaternion orientation as the ongoing rotation state.
        glm::vec3 GetRotationDegrees() const;
        
        
        // Immediately replaces the orientation using XYZ Euler angles in degrees.
        // Uses the original model convention: X acts first, then Y, then Z.
        // Does not add a turn, animate a transition, or apply look inversion.
        // Requires finite angle values.
        void SetRotationDegrees(const glm::vec3& rotationDegrees);
        // Immediately replaces the orientation using pitch=X, yaw=Y, roll=Z.
        // Uses exactly the same signs and order as the vec3 overload.
        // Does not apply look inversion.
        void SetRotationDegrees(const AcornData::EulerAnglesDegrees& angles);
        
        // Immediately replaces the orientation and normalizes the supplied quaternion.
        // Requires a finite, nonzero quaternion with a finite, nonzero squared length.
        // Does not change position or apply look inversion.
        void SetOrientation(const glm::quat& newOrientation);
        // Replaces orientation with a normalized copy of newOrientation.
        // Returns false and preserves the current orientation if its squared length
        // is zero or non-finite.
        // Unlike SetOrientation's assertions, this check also runs in release builds.
        bool TrySetOrientation(const glm::quat& newOrientation);
        
        // Replaces the camera's world position.
        // This is an absolute position, not an amount to move.
        // Does not change orientation. Supply finite coordinates.
        void SetPosition(const glm::vec3& newPosition);
        
        // Immediately aims the camera at a world-space target.
        // Uses upReference to choose the camera's roll, it does not need to match world up.
        // Returns false and preserves orientation if a usable orientation cannot be built,
        // including coincident positions or a viewing direction nearly parallel to upReference.
        // Does not move the camera, remember the target, or apply look inversion.
        // Call repeatedly to track a target.
        bool LookAt(const glm::vec3& target, glm::vec3 upReference = glm::vec3(0, 1, 0));
        
        
        // Adds a user-requested look change, measured in degrees.
        // Before inversion: positive X turns toward the camera's own right;
        // positive Y turns toward the camera's own up.
        // Applies lookControls, then local yaw, then pitch about the updated local right axis.
        // Works while tilted or upside down. No target is required.
        // Changes orientation only; does not read input devices, orbit, or clamp pitch.
        // Requires finite input values.
        void ApplyLookDelta(const glm::vec2& lookDegrees);
        
        
        // ---- Local Transform ----
        
        // Adds an XYZ-composed turn in the camera's local coordinate frame.
        // The supplied values are degrees, not a replacement orientation.
        // Uses the model XYZ composition order; does not add to stored Euler components.
        // A combined XYZ turn differs from separate local X, then Y, then Z calls.
        // Does not change position or apply look inversion.
        void RotateLocal(const glm::vec3& changeDegrees);
        // Adds the supplied quaternion turn in the camera's local coordinate frame.
        // Normalizes the current orientation and the change before combining them.
        // Requires valid, nonzero quaternions with finite, nonzero squared lengths.
        // Does not change position or apply look inversion.
        void RotateLocal(const glm::quat& change);
        
        // Adds movement along the camera's own axes, measured in world units.
        // Positive X moves right; positive Y moves up; negative Z moves forward.
        // These directions follow the camera's rotation, including roll.
        // Requires a unit orientation. Does not change orientation.
        void TranslateLocal(const glm::vec3& displacement);
        
        // Turns around the camera's own right axis, measured in degrees.
        // Positive values pitch toward the camera's own up.
        // Does not change position or apply look inversion.
        void RotateLocalX(float angleDegrees);
        // Moves along the camera's own right axis.
        // Positive distance moves right; negative distance moves left.
        void TranslateLocalX(float distance);
        // Turns around the camera's own up axis, measured in degrees.
        // Positive values turn left; negative values turn right.
        // Does not change position or apply look inversion.
        void RotateLocalY(float angleDegrees);
        // Moves along the camera's own up axis.
        // Positive distance moves up; negative distance moves down.
        void TranslateLocalY(float distance);
        // Turns around local +Z, which points backward from the camera.
        // Positive values tilt the camera's up direction toward its own left.
        // Does not change position or apply look inversion.
        void RotateLocalZ(float angleDegrees);
        // Moves along the camera's local Z axis.
        // Positive distance moves backward; negative distance moves forward.
        void TranslateLocalZ(float distance);
        
        
        // ---- World Transform ----
        
        // Adds an XYZ-composed turn expressed in the fixed world coordinate frame.
        // The supplied values are degrees, not a replacement orientation.
        // World axes do not follow the camera's current rotation.
        // Does not move the camera around the world origin or apply look inversion.
        void RotateWorld(const glm::vec3& changeDegrees);
        // Adds the supplied quaternion turn expressed in the fixed world coordinate frame.
        // Normalizes the current orientation and the change before combining them.
        // Requires valid, nonzero quaternions with finite, nonzero squared lengths.
        // Changes orientation only; position remains unchanged.
        void RotateWorld(const glm::quat& change);
        // Adds movement along fixed world axes, measured in world units.
        // Camera orientation does not affect the movement direction.
        // Does not change orientation.
        void TranslateWorld(const glm::vec3& displacement);
        
        // Adds a turn around fixed world +X, measured in degrees.
        // Position remains unchanged; ignores look inversion.
        void RotateWorldX(float angleDegrees);
        // Adds distance to the world X position.
        // Positive moves toward world +X; negative moves toward world -X.
        void TranslateWorldX(float distance);
        // Adds a turn around fixed world +Y, measured in degrees.
        // This is not necessarily the camera's own up axis.
        // Position remains unchanged; ignores look inversion.
        void RotateWorldY(float angleDegrees);
        // Adds distance to the world Y position.
        // Positive moves toward world +Y; negative moves toward world -Y.
        void TranslateWorldY(float distance);
        // Adds a turn around fixed world +Z, measured in degrees.
        // Position remains unchanged; ignores look inversion.
        void RotateWorldZ(float angleDegrees);
        // Adds distance to the world Z position.
        // This is not necessarily forward or backward relative to the camera.
        void TranslateWorldZ(float distance);
        
        
        // ---- Info ----
        
        // Stores the camera's viewport rectangle in framebuffer pixels.
        // Width and height must be nonnegative; zero is allowed when no area is drawable.
        // Does not call glViewport or change any OpenGL state.
        void SetViewport(int x, int y, int width, int height);
        // Returns true when both viewport width and height are greater than zero.
        // Check this before requesting projection or drawing.
        // Does not validate the projection settings.
        bool HasValidViewport() const;
        // Returns viewport width divided by viewport height.
        // Requires HasValidViewport() to be true.
        // Requires a viewport with positive width and height.
        // Calculates the ratio from the viewport rather than storing a separate copy.
        // Delegates the calculation to Rendering::GetAspectRatio.
        float GetAspectRatio() const;
        // Builds the world-to-camera view matrix from position and orientation.
        // Includes camera roll and uses the camera's own up direction.
        // Requires finite position and a unit orientation.
        glm::mat4 GetViewMatrix() const;
        // Builds projection using projectionSettings and the viewport's aspect ratio.
        // Requires a valid viewport and valid settings for the selected projection type.
        // Does not change OpenGL state.
        glm::mat4 GetProjectionMatrix() const;
        // Calculates and returns the current view and projection matrices.
        // Requires the same valid pose, viewport, and projection settings
        // as GetViewMatrix() and GetProjectionMatrix().
        [[nodiscard]] CameraMatrices GetMatrices() const;
        
    };
    
    
    
    // Builds a right-handed world-to-camera view matrix.
    // Uses the camera's position, forward direction, and own up direction.
    // Includes roll; does not need a separately stored look-at target.
    // Requires finite position and a unit orientation.
    glm::mat4 BuildViewMatrix(const CameraData& camera);
    
    
    // Builds perspective or orthographic projection from the supplied settings.
    // Currently follows GLM's configured handedness and depth convention.
    // With default GLM settings: right-handed, with normalized depth from -1 to +1.
    // Requires finite aspectRatio > 0 and finite clipping distances with 0 < near < far.
    // Perspective requires finite vertical FOV strictly between 0 and 180 degrees.
    // Orthographic projection requires finite orthographicHeight > 0.
    // Does not change OpenGL state.
    glm::mat4 BuildProjectionMatrix(const ProjectionSettings& settings, float aspectRatio);
    
    
    // Returns width divided by height as a floating-point aspect ratio.
    // Supply the dimensions of the area being rendered, using the same units.
    // Both dimensions must be greater than zero.
    // For split screen, use the individual viewport dimensions,
    // not the dimensions of the entire framebuffer.
    float GetAspectRatio(int frameBufferWidth, int frameBufferHeight);
    
    
}


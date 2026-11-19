#include "Camera.h"


// Standard library
#include <cassert>
#include <cmath>

// Third-party libraries
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>



namespace Rendering
{
    

    CameraData::CameraData(const glm::vec3& startingPosition, const glm::vec3& startingRotationDegrees)
        : position(startingPosition), orientation(AcornMath::MakeOrientationXYZ(startingRotationDegrees))
    {
    }

    CameraData::CameraData(const glm::vec3& startingPosition, const AcornData::EulerAnglesDegrees& startingAngles)
        : position(startingPosition), orientation(AcornMath::MakeOrientationXYZ(startingAngles))
    {
    }

    CameraData::CameraData(const glm::vec3& startingPosition, const glm::quat& startingOrientation)
        : position(startingPosition)
    {
        SetOrientation(startingOrientation);
    }
    
    

    glm::vec3 CameraData::GetForward() const { return AcornMath::GetForwardDirection(orientation); }
    glm::vec3 CameraData::GetRight() const { return AcornMath::GetRightDirection(orientation); }
    glm::vec3 CameraData::GetUp() const { return AcornMath::GetUpDirection(orientation); }
    glm::vec3 CameraData::GetRotationDegrees() const { return AcornMath::GetRotationXYZDegrees(orientation); }
    
    void CameraData::SetRotationDegrees(const glm::vec3& degrees) { orientation = AcornMath::MakeOrientationXYZ(degrees); }
    void CameraData::SetRotationDegrees(const AcornData::EulerAnglesDegrees& angles) { orientation = AcornMath::MakeOrientationXYZ(angles); }

    void CameraData::SetOrientation(const glm::quat& newOrientation)
    {
        const float lengthSquared = glm::dot(newOrientation, newOrientation);
        
        assert(std::isfinite(lengthSquared) && lengthSquared > 0.0f);
        orientation = glm::normalize(newOrientation);
    }

    void CameraData::SetPosition(const glm::vec3& newPosition) { position = newPosition; }

    
    
    bool CameraData::LookAt(const glm::vec3& target, glm::vec3 upReference)
    {
        return AcornMath::TryMakeLookOrientation(position, target, orientation, upReference);
    }
    
    
    void CameraData::ApplyLookDelta(const glm::vec2& lookDegrees)
    {
        assert( std::isfinite(lookDegrees.x) && std::isfinite(lookDegrees.y));

        // The caller supplies positive X for right and positive Y for up.
        const float horizontalDegrees = lookControls.invertHorizontal
            ? -lookDegrees.x
            : lookDegrees.x;

        const float verticalDegrees = lookControls.invertVertical
            ? -lookDegrees.y
            : lookDegrees.y;

        // Our -Z-forward camera turns right around negative local Y.
        RotateLocalY(-horizontalDegrees);

        // Positive local X pitches upward, using the updated axes.
        RotateLocalX(verticalDegrees);
    }
    
    
    
    // ---- Local Transform ----

    
    void CameraData::RotateLocal(const glm::vec3& changeDegrees)
    {
        AcornMath::RotateLocal(orientation, changeDegrees);
    }
    
    void CameraData::RotateLocal(const glm::quat& change)
    {
        AcornMath::RotateLocal(orientation, change);
    }

    void CameraData::TranslateLocal(const glm::vec3& displacement)
    {
        AcornMath::TranslateLocal(position, orientation, displacement);
    }

    void CameraData::RotateLocalX(float angleDegrees)
    {
        RotateLocal(glm::vec3(angleDegrees, 0.0f, 0.0f));
    }

    void CameraData::TranslateLocalX(float distance)
    {
        TranslateLocal(glm::vec3(distance, 0.0f, 0.0f));
    }

    void CameraData::RotateLocalY(float angleDegrees)
    {
        RotateLocal(glm::vec3(0.0f, angleDegrees, 0.0f));
    }

    void CameraData::TranslateLocalY(float distance)
    {
        TranslateLocal(glm::vec3(0.0f, distance, 0.0f));
    }

    void CameraData::RotateLocalZ(float angleDegrees)
    {
        RotateLocal(glm::vec3(0.0f, 0.0f, angleDegrees));
    }

    void CameraData::TranslateLocalZ(float distance)
    {
        TranslateLocal(glm::vec3(0.0f, 0.0f, distance));
    }
    
    
    
    // ---- World Transform ----

    void CameraData::RotateWorld(const glm::vec3& changeDegrees)
    {
        AcornMath::RotateWorld(orientation, changeDegrees);
    }

    void CameraData::RotateWorld(const glm::quat& change)
    {
        AcornMath::RotateWorld(orientation, change);
    }

    void CameraData::TranslateWorld(const glm::vec3& displacement)
    {
        AcornMath::TranslateWorld(position, displacement);
    }

    void CameraData::RotateWorldX(float angleDegrees)
    {
        RotateWorld(glm::vec3(angleDegrees, 0.0f, 0.0f));
    }

    void CameraData::TranslateWorldX(float distance)
    {
        TranslateWorld(glm::vec3(distance, 0.0f, 0.0f));
    }

    void CameraData::RotateWorldY(float angleDegrees)
    {
        RotateWorld(glm::vec3(0.0f, angleDegrees, 0.0f));
    }

    void CameraData::TranslateWorldY(float distance)
    {
        TranslateWorld(glm::vec3(0.0f, distance, 0.0f));
    }

    void CameraData::RotateWorldZ(float angleDegrees)
    {
        RotateWorld(glm::vec3(0.0f, 0.0f, angleDegrees));
    }

    void CameraData::TranslateWorldZ(float distance)
    {
        TranslateWorld(glm::vec3(0.0f, 0.0f, distance));
    }
    
    
    
    // ---- Info ----

    void CameraData::SetViewport(int x, int y, int width, int height)
    {
        assert(width >= 0 && height >= 0);
        viewport = Viewport{x, y, width, height};
    }

    bool CameraData::HasValidViewport() const
    {
        return viewport.width > 0 && viewport.height > 0;
    }

    float CameraData::GetAspectRatio() const
    {
        assert(HasValidViewport());
        return static_cast<float>(viewport.width) / static_cast<float>(viewport.height);
    }

    glm::mat4 CameraData::GetViewMatrix() const { return BuildViewMatrix(*this); }

    glm::mat4 CameraData::GetProjectionMatrix() const
    {
        return BuildProjectionMatrix(projectionSettings, GetAspectRatio());
    }
    
    
    
    
    glm::mat4 BuildViewMatrix(const CameraData& camera)
    {
        // glm::lookAtRH always builds a right-handed view matrix, glm::lookAtLH would build a left-handed 
        // view matrix, and glm::lookAt(...)	Uses GLM's configured handedness; right-handed by default
        return glm::lookAtRH(camera.position, camera.position + camera.GetForward(), camera.GetUp());
    }


    glm::mat4 BuildProjectionMatrix(const ProjectionSettings& settings, const float aspectRatio)
    {
        assert(std::isfinite(aspectRatio) && aspectRatio > 0.0f);
        assert(std::isfinite(settings.nearPlane) && std::isfinite(settings.farPlane));
        assert(settings.nearPlane > 0.0f && settings.farPlane > settings.nearPlane);
        
        switch (settings.projectionType)
        {
            case ProjectionType::PERSPECTIVE:
            {
                assert(std::isfinite(settings.verticalFovDegrees));
                assert(settings.verticalFovDegrees > 0.0f);     // assert(settings.verticalFovDegrees > 0.0f && settings.verticalFovDegrees < 180.0f);
                return glm::perspective(
                    glm::radians(settings.verticalFovDegrees), 
                    aspectRatio, 
                    settings.nearPlane, settings.farPlane);                    
            }            
            
            case ProjectionType::ORTHOGRAPHIC:
            {
                assert(std::isfinite(settings.orthographicHeight) && settings.orthographicHeight > 0.0f);
                const float halfHeight = settings.orthographicHeight * 0.5f;
                const float halfWidth = halfHeight * aspectRatio;
                
                return glm::ortho(
                    -halfWidth, 
                    halfWidth, 
                    -halfHeight,
                    halfHeight,
                    settings.nearPlane,
                    settings.farPlane);                    
            }
        }
        
        assert(false && "Unknown projection type");
        return glm::mat4{1.0f};
    }
    
    
}

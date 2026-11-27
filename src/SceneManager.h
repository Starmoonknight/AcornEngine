#pragma once



// Project headers
#include "assets/AssetRecords.h"
#include "assets/ModelCache.h"
#include "rendering/Camera.h"
#include "rendering/RenderInstances.h"
#include "testing/Player.h"
#include "Palette.h"

// Standard library
#include <array>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

// Third-party libraries
#include <glm/glm.hpp>     
#include <flecs.h>



class ShaderProgram;

namespace Assets
{
    class FlatModelAssets;
}



namespace AcornData
{
    
    
    struct Sun
    {
        glm::vec3 lightColor{1.0f};
        float lightStrength = 1.0f;
        float sunAngleDegrees = 0.0f;
    };


    // Initial world-space pose for one player.
    // rotationDegrees uses the existing XYZ convention: X acts first,
    // then Y, then Z. An unrotated camera faces -Z.
    // These starting values do not change when the player moves.
    struct PlayerStart
    {
        glm::vec3 position{0.0f};
        glm::vec3 rotationDegrees{0.0f};
    };

    enum class ESplitScreenSettings
    {
        VerticalSplit,              // Vertical dividing lines: views arranged left to right.
        HorizontalSplit,            // Horizontal dividing lines: views arranged bottom to top.
        GridSplit,                  // Discord or Teams similar layout 
    };


    struct SceneSettings
    {
        glm::vec3 defaultBackgroundColor{Palette::paleSkyBlue};
        
        Sun sun{}; 
        
        ESplitScreenSettings splitScreen = ESplitScreenSettings::VerticalSplit;
    };

    
    // Instructions for creating one placed model.
    // Contains source information and initial values, not GPU resources.
    // Loading copies transform/appearance into a runtime render instance.
    //
    // Textures are not described here yet. AddObject can receive a borrowed
    // texture handle separately. A future file format should identify
    // textures by path or asset name.
    struct SceneObjectDescription
    {
        Assets::ModelSource source;
        Transform transform; 
        ModelRenderSettings appearance;
        
        std::string textureFilePath;
    };

    // Description of a level before it is instantiated.
    // Does not own runtime objects, cameras, or uploaded GPU resources.
    //
    // Runtime does not modify this description.
    struct SceneDescription
    {
        std::string name;
        int sceneID = -1;
        SceneSettings settings;

        std::array<PlayerStart, 2> playerStarts;
        glm::vec3 goalPosition{0.0f};

        std::vector<SceneObjectDescription> objects;
    };
    
}




// temp for the player showcase 
struct SceneUpdateResult
{
    // The caller decides what level to load.
    // UpdateScene itself never clears or replaces the scene.
    bool goalReached = false;
};


struct FramebufferSize
{
    int width = 0;
    int height = 0;
};


// Scene-owned camera and per-view presentation settings.
// The current player convention is cameras[0] for player 1,
// cameras[1] for player 2. ID is a label, not automatic vector lookup.
struct SceneCamera
{
    int ID = -1;   // -1 means unassigned.
    bool enabled = true; 
    Rendering::CameraData camera;

    glm::vec3 backgroundColor{0.0f};
    
    // Temporary orbit-demo state. Ordinary player cameras do not use
    // these fields unless some orbit-update code is explicitly called.
    glm::vec3 target{0.0f};
    glm::vec3 startingOffset{4.0f, 5.0f, 7.0f};
    float orbitAngleDegrees = 0.0f;
    
    
    // temp-player code to skip rendering the player this camera is attached to 
    // Borrowed pointer. Skip this instance only when drawing this view.
    const AcornData::ModelInstance* hiddenInstance = nullptr;
};



// One placed model owned by Scene::placedObjects, this is a runtime placement.
struct SceneObject
{
    // Model path and import settings retained for reconstruction.
    // Does not currently describe the borrowed texture.
    Assets::ModelSource source;
    
    // Current placement, appearance, and borrowed resource references.
    // Stored by value: destroying SceneObject destroys this instance.
    // GPU geometry belongs to the application’s model cache.
    // The texture, if any, belongs to an external owner.
    AcornData::ModelInstance instance;
};




struct Scene
{
    // Owns uploaded geometry shared by instances in this scene.
    // Declare before all scene-owned objects that borrow its models:
    // members are destroyed in reverse declaration order.
    //
    // Clear the complete scene through Scene::Clear(), not by clearing
    // this cache while instances still use its models.
    //
    /*
    // Note to Self:    
    //      Keep this member before objects:
    //      C++ destroys members in reverse declaration order, so the cache
    //      will be destroyed after the instances that refer to it.
    // Second note: 
    //      Might be moved out of scene if cache should keep object used in next 
    //      scene instead of deleting everything on Scene switch. 
    //      Might make a pointer instead later?  
    */
    
    // Borrowed application resources. Scene does not destroy these.
    // They must outlive every scene and preview using them.
    Assets::ModelCache& modelCache;
    flecs::world& world;
    Assets::FlatModelAssets& flatModels;
    
    // Identity of the shared engine default material.
    AcornData::MaterialId fallbackMaterial{};
    
    
    // Fixed storage: body addresses remain stable while the Scene exists.
    std::array<Player, 2> players{};
    bool playersReady = false;
    
    // Allows one completion report after players enter the goal.
    // Starts false after loading. Becomes true when all active players
    // are outside the goal, then becomes false when entry is reported.
    // Spawning inside the goal therefore does not immediately finish it.
    bool goalArmed = false;
    
    
    // Owns static/placed objects and their render instances.
    // Appending preserves existing object addresses.
    // This design currently supports appending and clearing the whole
    // collection. Middle erasure can invalidate other object addresses.
    std::deque<SceneObject> placedObjects;
    
    // Non-owning drawing registrations.
    // May refer to placed objects or, later, bodies owned by players.
    // Registration does not create an object or extend its lifetime.
    std::vector<AcornData::ModelInstance*> renderInstances;
    
    // Actual camera values owned by this scene.
    // Clear or vector reallocation can invalidate camera references.
    std::vector<SceneCamera> cameras;
    
    AcornData::SceneSettings settings;
    
    std::string name;
    int sceneID = -1;       // Application-assigned label; -1 means unassigned.
    
    
    
    // Initial poses retained independently of moving player/camera state.
    std::array<AcornData::PlayerStart, 2> playerStarts{};
    
    // Runtime goal location used by the goal-completion check.
    glm::vec3 goalPosition{0.0f};
    
    
    
    // Create an empty scene using application-owned asset services.
    // Construction does not load geometry or create placed objects.
    Scene(
        Assets::ModelCache& cache,
        flecs::world& ecs,
        Assets::FlatModelAssets& models,
        AcornData::MaterialId defaultMaterial)
        : modelCache(cache),
          world(ecs),
          flatModels(models),
          fallbackMaterial(defaultMaterial)
    {
    }
    
    
    // Automatic member destruction releases scene-owned resources.
    // Scene borrows the cache, application construction order controls its lifetime.
    // A suitable OpenGL context must remain current during destruction.
    //
    // Copying is prohibited because borrowed registrations would otherwise
    // still point into the original scene. Moving is also prohibited by
    // this implementation; do not relocate a populated Scene.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;
    
    
    // Load/reuse geometry, resolve its default model asset, and create a
    // scene-owned instance with a private material converted from description.
    //
    // texture identifies a shared Texture record; zero means none assigned.
    // 
    // Returns nullptr when geometry loading fails.
    // The returned object survives appends, but not Clear()/scene destruction.
    // Requires a current OpenGL context if geometry must be uploaded.
    SceneObject* AddObject(
        const AcornData::SceneObjectDescription& description,
        AcornData::TextureId texture = {});
    
    
    // Adds an existing instance to the drawing list.
    // Ignores null and duplicate addresses. Does not take ownership.
    // The owner must keep the instance at a valid address while registered.
    void RegisterRenderInstance(AcornData::ModelInstance* model); 
    
    // Removes all registrations of this address.
    // Does not destroy the instance or release its resources.
    // Unregister before destroying an individually owned instance.
    void UnregisterRenderInstance(AcornData::ModelInstance* model);
    
    
    // Remove scene-owned instances and borrowed render registrations.
    // Destroy scene-owned runtime contents.
    // Shared geometry and asset records remain alive in application-owned storage.
    // Previously returned SceneObject pointers become invalid.
    //  Note:   If I get any event system up and running maybe make this an event 
    //          so other scripts can know when pointers are stale? 
    void Clear();
    
    
    // Assigns framebuffer-pixel rectangles to enabled cameras.
    // Clears disabled cameras' rectangles. Zero drawable area produces
    // no valid viewports. Does not call glViewport or draw anything.
    // Reconfigure after loading, resizing, or changing enabled cameras.
    void ConfigureSceneViewports(const FramebufferSize& framebuffer);
    
    
    // Note: temp for player showcase 
    // Applies player actions and checks goal completion.
    // Does not poll devices, draw, or load another scene.
    //
    // multiplayerEnabled is application state, so it survives scene changes.
    // skipGameplay preserves freshly loaded poses and skips goal detection
    // on a frame in which the caller has already replaced the scene.
    SceneUpdateResult UpdateScene(
        float deltaTime,
        const std::array<Input::PlayerInput, 2>& playerInputs,
        bool multiplayerEnabled,
        bool skipGameplay);
    
    
    // Clears the framebuffer and draws registered instances through each
    // enabled camera with a valid viewport. 
    // Does not advance gameplay.
    // Uses each camera's background color and calculated matrices.
    // Changes OpenGL state; disables scissor testing before returning.
    // Note: Model matrices are currently recalculated for each camera.
    void DrawScene(const ShaderProgram& shader);
    
};




// Replaces destination with a runtime scene built from description.
// Uses two scene-owned cameras as temporary player viewpoints.
// Objects are currently created without textures.
//
// Reported model-load failure clears the partial scene and returns false;
// the previous scene is not restored. 
// Allocation exceptions are not converted into false.
//
// description must remain valid throughout the call.
// Requires a suitable current OpenGL context.
// Call ConfigureSceneViewports afterward, before drawing.
bool LoadSceneFromDescription(
    Scene& destination,
    const AcornData::SceneDescription& description);









// Note:    Code can still bypass intended usage rules for Scene's with objects.clear() or 
//          modelCache.Clear() at the moment. Look into encapsulation improvements later. 

#pragma once

// Project headers
#include "Model.h"
#include "rendering/RenderInstances.h"

// Standard libraries
#include <unordered_map>
#include <stdexcept>

// Third-party libraries
#include <flecs.h>


namespace Assets
{
    
    // Look up component T on an existing entity.
    // Returns nullptr for an unassigned ID, dead entity, or missing component.
    //
    // The returned pointer is borrowed from Flecs.
    // Use it during the current operation; retain the entity ID for later
    // access instead of retaining this pointer across structural ECS changes.
    template<class T>
    const T* Find(flecs::world& world, flecs::entity_t id)
    {
        if (id == 0)
            return nullptr;

        const auto entity = world.entity(id);

        if (!entity.is_alive())
            return nullptr;

        return entity.try_get<T>();
    }
    
    // Create a texture record in Flecs and return its identity.
    // This copies the record, not the GPU image.
    // The existing texture owner remains responsible for glDeleteTextures.
    inline AcornData::TextureId StoreTexture(
        flecs::world& world,
        const AcornData::Texture& texture)
    {
        auto entity = world.entity();
        
        // set() stores the component and returns a builder for chaining.
        // Casting the result to void explicitly discards that return value;
        // the function still executes normally and stores the component.
        (void)entity.set<AcornData::Texture>(texture);
        
        return {entity.id()};
    }
    
    // Create an independent material record.
    // Its texture reference still points to the same shared texture.
    inline AcornData::MaterialId StoreMaterial(
        flecs::world& world,
        const AcornData::Material& material)
    {
        auto entity = world.entity();
        (void)entity.set<AcornData::Material>(material);
        return {entity.id()};
    }
    
    // Copy the model definition into Flecs and return its identity.
    // Nodes and slots are copied; referenced GPU geometry and materials
    // remain shared. The geometry owner must outlive this asset.
    //
    // Store a structurally valid model definition.
    // Throws before creating an entity if the definition is invalid.
    // Geometry lifetime and material-entity validity remain caller obligations.
    inline AcornData::ModelAssetId StoreModel(
        flecs::world& world,
        const AcornData::ModelAsset& asset)
    {
        std::string error;
        if (!asset.Validate(error))
            throw std::runtime_error(error);

        auto entity = world.entity();
        (void)entity.set<AcornData::ModelAsset>(asset);
        return {entity.id()};
    }
    
    
    
    
    // Convert the old per-object appearance fields into material data.
    // This is a migration adapter for existing scenes and demos.
    //
    // Old tintAlpha measures material-color contribution.
    // New vertexColorWeight measures the opposite contribution.
    // Texture references are borrowed/shared, not duplicated.
    inline AcornData::Material MakeMaterial(
        const AcornData::ModelRenderSettings& old,
        AcornData::TextureId texture)
    {
        AcornData::Material material;
        material.baseColor = old.tintColor;
        material.diffuseTexture = texture;

        material.vertexColorWeight =
            old.overrideColor ? 1.0f - old.tintAlpha : 1.0f;

        material.textureTintStrength = old.tintTextureAlpha;
        material.textureExpected = old.textureExpected;

        return material;
    }
    
    
    // application/session lookup for default model assets.
    // This lookup is a cache of IDs, not a second asset owner.
    // Requires geometry cache to stay alive and not be cleared during the session.
    class FlatModelAssets
    {
    public:
        
        // Return the default flat asset already associated with this geometry,
        // or build, validate, and store one on the first request.
        //
        // Geometry addresses are cache keys. The geometry cache must remain
        // alive and must not be cleared while this lookup is in use.
        // All calls on this object must use the same Flecs world.
        //
        // name and fallback are used only when creating the first asset.
        // Customized presets must be stored separately, not through this lookup.
        AcornData::ModelAssetId GetOrCreate(
            flecs::world& world,
            const AcornData::GpuModel& geometry,
            const std::string& name,
            AcornData::MaterialId fallback)
        {
            if (const auto found = ids.find(&geometry);
                found != ids.end())
            {
                return found->second;
            }

            auto asset =
                MakeFlatModelAsset(geometry, name, fallback);

            std::string error;
            if (!asset.Validate(error))
                throw std::runtime_error(error);

            const auto id = StoreModel(world, asset);
            
            // Cache only the asset identity, Flecs owns the ModelAsset record.
            ids.emplace(&geometry, id);
            return id;
        }
        

    private:
        std::unordered_map<
            const AcornData::GpuModel*,
            AcornData::ModelAssetId> ids;
    };
    
    
    
}

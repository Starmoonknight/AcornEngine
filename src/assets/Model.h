#pragma once

// Project headers
#include "AcornMath.h"
#include "rendering/Material.h"

// Standard libraries 
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

// Third-party libraries
#include <flecs.h>
#include <glm/mat4x4.hpp> 




namespace AcornData
{
    
    struct GpuModel;
    
    
    
    struct ModelAssetId
    {
        // Flecs entity containing ModelAsset.
        // Zero means no model asset is assigned.
        flecs::entity_t value = 0;
    };
    
    
    struct MaterialSlot
    {
        std::string name;
        MaterialId material;
    };
    
    
    
    struct ModelPart
    {
        std::size_t meshIndex = 0;          // Selects an entry in the referenced GpuModel::meshes.
        std::size_t materialSlotIndex = 0;  // Selects an entry in the asset's material slots.
    };


    // Note: If two pieces must move independently, give them separate nodes.
    struct ModelNode
    {
        std::string name;
        std::optional<std::size_t> parentIndex; // Selects another node in this same asset. No parent means the node is a root.
        Transform localTransform{};             // Places the node relative to its parent.
        std::vector<ModelPart> parts;           // Contains the geometry/material assignments drawn at that node.
    };
    
    
    struct ModelAsset
    {
        std::string name = "Unnamed"; 
        
        // Borrowed from ModelCache.
        const GpuModel* geometry = nullptr;

        Transform assetTransform{};             // Adjustment to the complete asset
        std::vector<MaterialSlot> materialSlots;
        std::vector<ModelNode> nodes;

        // Un-assigned means use the normal part assignments.
        MaterialId wholeMaterialOverride{};
        
        // Check structural references before rendering:
        // geometry exists, nodes exist, parents precede children,
        // and every part refers to a valid mesh and material slot.
        //
        // Returns false with an explanation in error.
        // Does not check whether referenced material entities are alive,
        // and cannot detect a dangling geometry pointer.
        bool Validate(std::string& error) const;

        // Calculate one world-space drawing matrix for every node.
        // output[i] corresponds to nodes[i].
        //
        // instanceTransform places this particular model in the world.
        // assetTransform adjusts the complete shared model.
        // Each node's local transform places it relative to its parent.
        //
        // localOverrides must be empty or have exactly nodes.size() entries.
        // A populated override replaces that node's default local transform.
        //
        // Requires a structurally valid asset with parents before children.
        // Replaces output contents; does not change the asset or instance.
        void BuildNodeMatrices(
            const Transform& instanceTransform,
            std::span<const std::optional<Transform>> localOverrides,
            std::vector<glm::mat4>& output) const;
    };
    
    
}




namespace Assets
{
    // Build an editable model definition around already-uploaded geometry.
    // Creates one identity root and one identity child per GPU mesh.
    // Each child draws its mesh using the same default material slot.
    //
    // "Flat" means every mesh node has the root as its direct parent.
    // This preserves the existing flat renderer's placement; it does not
    // recover hierarchy or material assignments from the original file.
    //
    // IMPORTANT: Borrows geometry, its owner must outlive the returned asset! 
    AcornData::ModelAsset MakeFlatModelAsset(
        const AcornData::GpuModel& geometry,
        std::string name,
        AcornData::MaterialId defaultMaterial);
    
    
}



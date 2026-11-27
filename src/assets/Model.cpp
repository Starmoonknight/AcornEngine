#include "Model.h"

// Project headers
#include "rendering/GpuModel.h"

// Standard libraries 
#include <cassert>
#include <utility>


namespace AcornData
{
    
    bool ModelAsset::Validate(std::string& error) const
    {
        // Validate:
        error.clear(); // Remove a previous failure message on reuse.

        // A parent index smaller than i is both in range and already evaluated.
        // Requiring this order also makes hierarchy cycles impossible.

        // Every drawable part needs a valid geometry selection and slot.
        // A slot may intentionally contain MaterialId{0}; rendering then falls back.
        
        if (geometry == nullptr)
        {
            error = "Model has no geometry.";
            return false;
        }
        
        if (nodes.empty())
        {
            error = "Model has no nodes.";
            return false;
        }
        
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            const ModelNode& node = nodes[i];

            // Parents must appear before children.
            // This also prevents self-parenting and cycles.
            if (node.parentIndex && *node.parentIndex >= i)
            {
                error = "Node parent must precede its child.";
                return false;
            }

            for (const ModelPart& part : node.parts)
            {
                if (part.meshIndex >= geometry->meshes.size())
                {
                    error = "Part references an invalid mesh.";
                    return false;
                }

                if (part.materialSlotIndex >= materialSlots.size())
                {
                    error = "Part references an invalid material slot.";
                    return false;
                }
            }
        }

        return true;
    }

    
    void ModelAsset::BuildNodeMatrices(
        const Transform& instanceTransform,
        std::span<const std::optional<Transform>> localOverrides, 
        std::vector<glm::mat4>& output) const
    {
        // These are programmer preconditions, not file-input validation.
        // assert() may be disabled in release builds.
        // Empty means every node uses its asset default.
        assert(localOverrides.empty() ||
               localOverrides.size() == nodes.size());

        output.resize(nodes.size());

        // With the column-vector convention the rightmost matrix acts first:
        // apply the shared asset adjustment, then place this instance.
        const glm::mat4 modelWorld =
            AcornMath::BuildModelMatrix(instanceTransform) *
            AcornMath::BuildModelMatrix(assetTransform);
        
        // Parents precede children, so each parent's result is already ready.
        // part world matrix =
        //      instance placement * asset transform * accumulated node transforms
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            const ModelNode& node = nodes[i];

            // Use this instance's replacement pose when present.
            // Otherwise, inherit the shared asset's default pose.
            const Transform& local =
                !localOverrides.empty() && localOverrides[i]
                ? *localOverrides[i]
                : node.localTransform;

            // This matrix converts node-local coordinates to parent coordinates.
            const glm::mat4 localMatrix =
                AcornMath::BuildModelMatrix(local);

            if (node.parentIndex)
            {
                assert(*node.parentIndex < i);

                // Parent world matrix already includes instance placement,
                // asset adjustment, and every earlier ancestor transform.
                output[i] =
                    output[*node.parentIndex] * localMatrix;
            }
            else
            {
                // A root has no parent node. Attach it directly to the
                // complete model's world placement.
                output[i] = modelWorld * localMatrix;
            }
        }
    }
    
}




namespace Assets
{
    
    AcornData::ModelAsset MakeFlatModelAsset(const AcornData::GpuModel& geometry, std::string name,
        AcornData::MaterialId defaultMaterial)
    {
        AcornData::ModelAsset asset;
        asset.name = std::move(name);
        asset.geometry = &geometry; // Borrow the GPU collection; do not copy ownership.
        
        // One shared assignment point preserves the old one-material-per-model use.
        asset.materialSlots.push_back({
            "Default",
            defaultMaterial
        });

        // The identity root gives the mesh nodes a common parent.
        // It draws nothing itself.
        AcornData::ModelNode root;
        root.name = "Root";
        asset.nodes.push_back(std::move(root));


        // Each child starts at identity, preserving existing vertex placement.
        // Its local transform can subsequently move this mesh independently.

        // std::move transfers the node's string/vector storage into the asset.
        for (std::size_t mesh = 0;
             mesh < geometry.meshes.size();
             ++mesh)
        {
            AcornData::ModelNode node;
            node.name = "Mesh " + std::to_string(mesh);
            node.parentIndex = 0;
            node.parts.push_back({mesh, 0});

            asset.nodes.push_back(std::move(node));
        }

        return asset;
    }
    
    
}

